#include <types.h>
#include "syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <ports.h>

#define CONSOLE_WIDTH 80
#define CONSOLE_HEIGHT 25
#define ASCII_ESC 0x1b
#define PORT_HW_CURSOR 0x3D4

// Rueckgabewert bei den funtkionen zum Parsen der ANSI-Sequenzen
typedef enum {SUCCESS, NEED_MORE, INVALID} ansi_esc_seq_status_t;

// Farbe im Videospeicher
typedef union {
    byte raw;
    struct {
        unsigned char foreground : 3;
        unsigned char bold : 1;
        unsigned char background : 3;
        unsigned char blink : 3;
    } __attribute__((packed));
} __attribute__((packed)) color_t;

// Zelle im Videospeicher
typedef struct {
    char ascii;
    byte color;
} __attribute__((packed)) video_mem_cell_t;



/// Pointer auf den Videospeicher
video_mem_cell_t* video_memory;

/// Aktuelle Position im Videospeicher
int cursor_x = 0;
int cursor_y = 0;

/// Aktuelle Farben
color_t color = {.raw = 0x7};

// Anzahl der Zeichen im ANSI Buffer
int ansi_buf_ofs = 0;

// ANSI-Buffer
char ansi_buf[32];

// Wenn TRUE werden die Ausgaben auch auf die serielle Schnittstelle
// geschriben
static bool serial_output = TRUE;


///
void con_clear_screen();

///
void con_set_hw_cursor();



void con_init()
{
    // Ports fuer die serielle Schnittstelle reservieren
    if (!request_ports(0x3F8, 1)) {
        serial_output = FALSE;
    }
    if (serial_output && !request_ports(0x3FD, 1)) {
        serial_output = FALSE;
        release_ports(0x3F8, 1);
    }

    video_memory = mem_allocate_physical(CONSOLE_WIDTH * CONSOLE_HEIGHT *
        sizeof(video_mem_cell_t), 0xb8000, 0);
    
    if (request_ports(PORT_HW_CURSOR, 5) == FALSE) {
        puts("[VGATERM] Konnte Ports zum Steuern des Hardware-Cursors nicht reservieren!");
        exit(-1);
    }
   
    con_clear_screen();
    
}

/**
 * Setzt die Cursor-Position
 */
void con_set_cursor_position(unsigned int x, unsigned int y)
{
    if (x >= CONSOLE_WIDTH) {
        cursor_x = CONSOLE_WIDTH - 1;
    } else {
        cursor_x = x;
    }

    if (y >= CONSOLE_HEIGHT) {
        cursor_y = CONSOLE_HEIGHT - 1;
    } else {
        cursor_y = y;
    }

    con_set_hw_cursor();
}

/**
 * Leert den Bildschirm
 */
void con_clear_screen()
{
    int i;

    for (i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
        video_memory[i].ascii = 0;
        video_memory[i].color = color.raw;
    }
    
    con_set_cursor_position(0, 0);
}

/**
* Leer den Bildschirm von der aktuellen Cursor Position bis zur
* angegebenen Spalte
*
* @return Die Spalte bis zu der geleert wurde
*/
static unsigned int con_clear_to(unsigned int column)
{
    int i;

    for (i = cursor_x; i < column; i++) {
        video_memory[cursor_y * CONSOLE_WIDTH + i].ascii = 0;
        video_memory[cursor_y * CONSOLE_WIDTH + i].color = color.raw;
    }
    
    return column;
}

/**
 * Scrollt eine Zeile nach unten
 */
static void con_scroll_down()
{
    int i;
    
    // Inhalte um eine Zeile nach oben schieben
    memmove(video_memory, video_memory + CONSOLE_WIDTH, CONSOLE_WIDTH *
        (CONSOLE_HEIGHT - 1) * sizeof(video_mem_cell_t));
    
    // Unterste Zeile leeren
    for (i = CONSOLE_WIDTH * (CONSOLE_HEIGHT - 1); i < CONSOLE_WIDTH *
        CONSOLE_HEIGHT; i++)
    {
        video_memory[i].ascii = 0;
        video_memory[i].color = color.raw;
    }

    con_set_cursor_position(cursor_x, cursor_y - 1);
}

static ansi_esc_seq_status_t handle_ansi_formatting_sequence_number(int n)
{
    // Zum Umrechnen der ANSI Farbcodes in die VGA Palette
    static char colors[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };
    
    switch(n)
    {
        // Normal
        case 0:
            color.bold = 0;
            color.blink = 0;
            break;
        
        // Fett
        case 1:
            color.bold = 1;
            break;
        
        // Blinkend
        case 5:
            color.blink = 1;
            break;
        
        // Vordergrundfarbe
        case 30 ... 37:
            color.foreground = colors[n - 30];
            break;
        
        // Hintergrundfarbe
        case 40 ... 47:
            color.background = colors[n - 40];
            break;

        default:
            return INVALID;
    }

    return SUCCESS;
}

/**
 * Versucht eine ANSI-Escape-Code-Sequenz zu parsen. 
 *
 * @param ansi_buf Sequenz die ausgewertet werden soll
 * @param ansi_buf_len Anzahl der auszuwertenden Bytes
 * 
 * @return INVALID, wenn die ANSI-Code-Sequenz ung�ltig ist, SUCCESS, wenn die
 *    ANSI-Code-Sequenz erfolgreich ausgewertet wurde, NEED_MORE, wenn die 
 *    Sequenz unvollst�ndig ist, und noch weitere Zeichen zur Auswertung 
 *    eingelesen werden m�ssen.
 */
static ansi_esc_seq_status_t con_ansi_parse(const char* ansi_buf, unsigned int ansi_buf_len)
{
  int i;
  int n1 = 0, n2 = 0;
  int have_n1 = 0, have_n2 = 0, have_delimiter = 0;

  static int save_cursor_x;
  static int save_cursor_y;
    
  if(ansi_buf_len == 0)
  {
    return NEED_MORE;
  }

  if(ansi_buf[0] != ASCII_ESC)
  {
    return INVALID;
  }

  if(ansi_buf_len == 1)
  {
    return NEED_MORE;
  }

  if(ansi_buf[1] != '[')
  {
    return INVALID;
  }

  if(ansi_buf_len == 2)
  {
    return NEED_MORE;
  }

  for(i = 2; i < ansi_buf_len; i++)
  {
    switch(ansi_buf[i])
    {
      case '0' ... '9':
        if(!have_delimiter)
        {
          n1 = n1 * 10 + (ansi_buf[i] - '0');
          have_n1 = 1;
        }
        else
        {
          n2 = n2 * 10 + (ansi_buf[i] - '0');
          have_n2 = 1;
        }
        break; // n�chstes Zeichen auswerten

      case ';':
        if(have_delimiter)
        {
          return INVALID;
        }
        have_delimiter = 1;
        break; // n�chstes Zeichen auswerten


        case 'A': // ESC[#A - Cursor um # Zeilen nach oben bewegen
          if(have_delimiter || !have_n1 || have_n2)
          {
            return INVALID;
          }
          con_set_cursor_position(cursor_x, cursor_y - n1);
          return SUCCESS;

          case 'B': // ESC[#B - Cursor um # Zeilen nach unten bewegen
            if(have_delimiter || !have_n1 || have_n2)
            {
              return INVALID;
            }
            con_set_cursor_position(cursor_x, cursor_y + n1);
            return SUCCESS;

            case 'C': // ESC[#C - Cursor um # Zeilen nach rechts bewegen
              if(have_delimiter || !have_n1 || have_n2)
              {
                return INVALID;
              }
              con_set_cursor_position(cursor_x + n1, cursor_y);
              return SUCCESS;
                
              case 'D': // ESC[#D - Cursor um # Zeilen nach links bewegen
                if(have_delimiter || !have_n1 || have_n2)
                {
                  return INVALID;
                }
                con_set_cursor_position(cursor_x - n1, cursor_y);
                return SUCCESS;

                case 'H': // ESC[#;#H - Cursor zur Zeile # und Spalte # bewegen
      case 'f':
        if(!have_n1 || !have_n2)
        {
          return INVALID;
        }
                
        con_set_cursor_position(n1, n2);
        return SUCCESS;

        case 'J': // ESC[2J - Bildschirm leeren
          if(have_delimiter || !have_n1 || have_n2 || n1 != 2)
          {
            return INVALID;
          }
          con_clear_screen();
          return SUCCESS;

          case 'K': // ESC[K - Bis Zeile von der Cursor Position zum Ende leeren
            if(have_delimiter || have_n1 || have_n2)
            {
              return INVALID;
            }
                
            con_clear_to(CONSOLE_WIDTH);

            return SUCCESS;

            case 'm': // ESC[#m oder ESC[#;#m - Vorder- und Hintergrundfarbe �ndern, Fettschrift, Blinken
              if(!have_n1)
              {
                return INVALID;
              }
                
              if(handle_ansi_formatting_sequence_number(n1) != SUCCESS)
              {
                return INVALID;
              }

              if(have_n2)
              {
                if(handle_ansi_formatting_sequence_number(n2) != SUCCESS)
                {
                  return INVALID;
                }
              }

              return SUCCESS;

              case 's': // ESC[s - Cursor-Position speichern
                if(have_delimiter || have_n1 || have_n2)
                {
                  return INVALID;
                }
                save_cursor_x = cursor_x;
                save_cursor_y = cursor_y;

                return SUCCESS;

                case 'u': // ESC[u - Cursor-Position wiederherstellen
                  if(have_delimiter || have_n1 || have_n2)
                  {
                    return INVALID;
                  }
        
                  con_set_cursor_position(save_cursor_x, save_cursor_y);

                  return SUCCESS;

      default:
        return INVALID;
    }
  }

  return NEED_MORE;
}

/**
 * Gibt ein Zeichen auf der Konsole aus.
 */
void con_putc(const char c)
{
    static char last_char = 0;
    
    // Zeichen auf die serielle Schnittstelle schreiben
    if (serial_output == TRUE) {
        outb(0x3f8, c);
        while ((inb(0x3fd) & 0x20) == 0) {
            asm("nop");
        }
    }
    
    switch (c) {
        case '\n':
            if (last_char != '\r') {
                con_clear_to(CONSOLE_WIDTH);
            }
            cursor_x = 0;
            cursor_y++;
            break;
        
        case '\r':
            cursor_x = 0;
            break;
        
        case '\t':
            cursor_x = con_clear_to((cursor_x & ~7) + 8);
            break;

        default:
            video_memory[cursor_x + cursor_y * CONSOLE_WIDTH].ascii = c;
            video_memory[cursor_x + cursor_y * CONSOLE_WIDTH].color = color.raw;
            cursor_x++;
            break;
    }
    
    if (cursor_x >= CONSOLE_WIDTH) {
        cursor_y += cursor_x / CONSOLE_WIDTH;
        cursor_x %= CONSOLE_WIDTH;
    }
    

    while (cursor_y >= CONSOLE_HEIGHT) {
        con_scroll_down();
    }
    
    con_set_hw_cursor();
    
    last_char = c;
}

/**
 * Gibt ein Zeichen auf der Konsole aus. Wenn das Zeichen ein Escape-Zeichen
 * ist, wird es und die folgenden Zeichen in einem Puffer zwischen gespeichert,
 * bis entweder diese Zeichenfolge erfolgreich als ANSI-Escape-Code-Sequenz
 * ausgewertet wurde, oder feststeht, dass diese Sequenz ung�ltig ist. In 
 * diesem Fall werden alle Zeichen einschlie�lich des Escape-Zeichens auf der 
 * Konsole ausgegeben.
 */
void con_putc_ansi(const char c)
{
    ansi_esc_seq_status_t status;
    int i;

    if (c == ASCII_ESC || ansi_buf_ofs > 0) {
        ansi_buf[ansi_buf_ofs++] = c;

        status = con_ansi_parse(ansi_buf, ansi_buf_ofs);

        switch (status) {
            case NEED_MORE:
                if (ansi_buf_ofs <= sizeof(ansi_buf)) {
                    break;
                }
                // fall through

            case INVALID:
                for (i = 0; i < ansi_buf_ofs; i++) {
                    con_putc(ansi_buf[i]);
                }
                ansi_buf_ofs = 0;
                break;
            
            case SUCCESS:
                ansi_buf_ofs = 0;
                break;
        }
    } else {
        con_putc(c);
    }
}

/**
 * Leert den Puffer, in dem die Zeichen einer m�glichen ANSI-Escape-Code-
 * Sequenz zwischengespeichert werden. Diese Funktion muss aufgerufen werden,
 * wenn sichergestellt werden soll, dass s�mtliche Zeichen, die via 
 * con_putc_ansi ausgegeben werden sollen, tats�chlich ausgegeben werden.
 */
void flush_ansi_escape_code_sequence()
{
    int i;
    
    if (ansi_buf_ofs > 0) {
        for (i = 0; i < ansi_buf_ofs; i++) {
            con_putc(ansi_buf[i]);
        }
        ansi_buf_ofs = 0;
    }
}

/**
 * Gibt eine Zeichenkette auf der Konsole aus. Diese Zeichenkette kann
 * ANSI-Steuercodes enthalten. 
 * Unvollst�ndige und fehlerhafte Sequenzen werden einfach ausgegeben.
 */
void con_puts(const char* s)
{
    while (*s != 0) {
        con_putc_ansi(*s);
        s++;
    }

    flush_ansi_escape_code_sequence();
}

/**
 * Gibt eine Zeichenkette auf der Konsole aus. Diese Zeichenkette kann
 * ANSI-Steuercodes enthalten. Die Ausgabe terminiert bei Erreichen eines
 * Null-Bytes oder wenn n Bytes ausgewertet wurden.
 * Unvollst�ndige und fehlerhafte Sequenzen werden einfach ausgegeben.
 */
void con_putsn(unsigned int n, const char * s)
{
    while ((*s != 0) && (n-- != 0)) {
        con_putc_ansi(*s);
        s++;
    }

    flush_ansi_escape_code_sequence();
}

/**
 * Aktualisiert die Postition des Hardware Cursors
 */
void con_set_hw_cursor()
{
    // Hardware Cursor verschieben
    word hw_cursor_pos = cursor_x + cursor_y * CONSOLE_WIDTH;
    outb(PORT_HW_CURSOR, 15);
    outb(PORT_HW_CURSOR + 1, hw_cursor_pos);
    outb(PORT_HW_CURSOR, 14);
    outb(PORT_HW_CURSOR + 1, hw_cursor_pos >> 8);
}


