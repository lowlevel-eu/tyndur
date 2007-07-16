#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "collections.h"

#define _NO_STDIO_
#define _USE_START_
#include "init.h"
#include "lostio.h"

#define DEBUG_MSG(s,x) printf(s,x)
//#define DEBUG_MSG(s)

//#define SCREEN_WIDTH 80
//#define SCREEN_HEIGHT 25
#define ASCII_ESC 0x1b
#define INPUT_BUFFER_SIZE 1024

typedef struct 
{
    int id;
    int width;
    int height;
    unsigned short *buffer;
    int cursor_x;
    int cursor_y;
    int save_cursor_x;
    int save_cursor_y;
    char ansi_buf[16];
    char input_buffer[INPUT_BUFFER_SIZE];
    unsigned int ansi_buf_ofs;
    union
    {
      unsigned char _color;
      struct 
      {
        unsigned char foreground : 3;
        unsigned char bold : 1;
        unsigned char background : 3;
        unsigned char blink : 1;
      };
    } con_color;
} terminal_t;

list_t* terminals;
static int curr_terminal = 5;
static int terminal_ctr = 0;
static char* textbuffer = 0;

typedef enum ansi_esc_seq_status
{
  INVALID = 0,
  SUCCESS,
  NEED_MORE,
} ansi_esc_seq_status_t;

//terminal_t* terminal_create(int width, int height);
//void terminal_set_cursor(int x, int y);
//void terminal_puts(char chr);
    
size_t echo_write(lostio_filehandle_t *file, size_t blocksize, size_t blockcount, void *data);
size_t terminal_write(lostio_filehandle_t *file, size_t blocksize, size_t blockcount, char *data);
terminal_t* terminal_create();
void terminal_register(terminal_t *term);
terminal_t* terminal_getid(int id);
void terminal_change(int id);

int main(int argc, char* argv[])
{
    textbuffer = mem_allocate_physical(80*25*2, 0xb8000, 0);
    
    lostio_init();
    lostio_type_ramfile_use();
    lostio_type_directory_use();
    DEBUG_MSG("LostIO initialisiert",0);
    
    
    // echo-Typehandle
    typehandle_t* typehandle_echo = malloc(sizeof(typehandle_t));
    
    typehandle_echo->id          = 255;
    typehandle_echo->not_found   = NULL;
    typehandle_echo->pre_open    = NULL;
    typehandle_echo->post_open   = NULL;
    typehandle_echo->read        = NULL;
    typehandle_echo->write       = &echo_write; // auf stdout wird geschrieben
    typehandle_echo->seek        = NULL;
    typehandle_echo->close       = NULL;
    lostio_register_typehandle(typehandle_echo);
    
    // terminal-Typehandle
    typehandle_t* typehandle_terminal = malloc(sizeof(typehandle_t));
    typehandle_terminal->id          = 254;
    typehandle_terminal->not_found   = NULL;
    typehandle_terminal->pre_open    = NULL;
    typehandle_terminal->post_open   = NULL;
    typehandle_terminal->read        = NULL;
    typehandle_terminal->write       = &terminal_write; // auf stdout wird geschrieben
    typehandle_terminal->seek        = NULL;
    typehandle_terminal->close       = NULL;
    lostio_register_typehandle(typehandle_terminal);
    
    vfstree_create_node("/echo",255,0,NULL,0);

    // Zu beginn X terminals erstellen
    terminals = list_create();
    int start_terminals = 0;
    start_terminals = atoi(argv[0]);
    if (start_terminals <= 0) start_terminals = 10;
    
    int i = 0;
    for (i = 0; i < start_terminals; i++)
    {
        terminal_t *term = terminal_create();
        terminal_register(term);
    }
    
    DEBUG_MSG("Module loaded.",0);
    init_service_register("vterm");
    
    //terminal_t* current_term = 0;
    //void* textbuffer = 0;
    //textbuffer = mem_allocate_physical(80*25*2, (unsigned short*)0xb8000, 0);
    //printf("textbuffer = %d",textbuffer);
    //terminal_change(5);
    while(TRUE) {
        wait_for_rpc();
	/*int i;
        current_term = terminal_getid(curr_terminal++);
        for (i = 0; i < (current_term->height); i++)
            strncpy(textbuffer, current_term->buffer + (i * current_term->width * 2), current_term->width * 2);
        for (i = 0; i < 1000; i++);
        
        */
	//terminal_change(curr_terminal); 
	//for (i = 0; i < 1000; i++);
	//curr_terminal++;
        //if (curr_terminal > start_terminals) curr_terminal=0;
    }
}

size_t echo_write(lostio_filehandle_t *file, size_t blocksize, size_t blockcount, void *data)
{
    /*int i = 0;
    for (; i < blocksize * blockcount; i++)
    {
        putchar(*(data+i));
    }*/

    char *buffer = malloc(blocksize * blockcount + 1);
    strncpy(buffer, data, blocksize*blockcount);
    buffer[blocksize * blockcount] = '\0';
    printf("%s", buffer);
    free(buffer);

    return blocksize * blockcount;
}

/**
 * Setzt die Cursor-Position
 */
static void con_set_cursor_pos(terminal_t *term, unsigned int x, unsigned int y)
{
  if(x >= term->width)
  {
    term->cursor_x = term->width - 1;
  }
  else
  {
    term->cursor_x = x;
  }

  if(y >= term->height)
  {
    term->cursor_y = term->height - 1;
  }
  else
  {
    term->cursor_y = y;
  }
  /* Aktuell inexistent
  con_set_hw_cursor(); */
}

/**
 * Leert den Bildschirm
 */
static void con_clear_screen(terminal_t *term)
{
  int i;

  for(i = 0; i < term->width * term->height; i++)
  {
    term->buffer[i] = term->con_color._color << 8;
  }
    
  con_set_cursor_pos(term, 0, 0);
}

/**
 * Scrollt eine Zeile nach unten
 */
static void scroll_down(terminal_t *term)
{
  int i;
    
  memmove(term->buffer, term->buffer + term->width, term->width * (term->height - 1) * 2);

  for(i = term->width * (term->height - 1); i < term->width * term->height; i++)
  {
    term->buffer[i] = term->con_color._color << 8;
  }

  con_set_cursor_pos(term, term->cursor_x, term->cursor_y - 1);
}

static ansi_esc_seq_status_t handle_ansi_formatting_sequence_number(terminal_t *term, int n)
{
  static char colors[8] = { 0, 4, 2, 6, 1, 5, 3, 7 }; /* Zum Umrechnen der ANSI Farbcodes in die VGA Palette. */

  switch(n)
  {
    case 0: // Normal
      term->con_color.bold = 0;
      term->con_color.blink = 0;
      break;
        
      case 1: // Fett
        term->con_color.bold = 1;
        break;

      case 5: // Blinken
        term->con_color.blink = 1;
        break;

      case 30 ... 37: // Vordergrundfarbe
        term->con_color.foreground = colors[n - 30];
        break;

      case 40 ... 47: // Hintergrundfarbe
        term->con_color.background = colors[n - 40];
        break;

      default:
        return INVALID;
  }

  return SUCCESS;
}

/**
 * L�scht den Bildschirm von der aktuellen Cursor Position bis zur
 * angegebenen Spalte
 *
 * @return Die Spalte bis zu der gel�scht wurde
 */
static unsigned int con_clear_to(terminal_t *term, unsigned int column)
{
  int i;

  for(i = term->cursor_x; i < column; i++)
  {
    term->buffer[term->cursor_y * term->width + i] = term->con_color._color << 8;
  }

  return column;
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
static ansi_esc_seq_status_t con_ansi_parse(terminal_t *term, const char * ansi_buf, unsigned int ansi_buf_len)
{

  int i;
  int n1 = 0, n2 = 0;
  int have_n1 = 0, have_n2 = 0, have_delimiter = 0;
    
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
          con_set_cursor_pos(term, term->cursor_x, term->cursor_y - n1);
          return SUCCESS;

          case 'B': // ESC[#B - Cursor um # Zeilen nach unten bewegen
            if(have_delimiter || !have_n1 || have_n2)
            {
              return INVALID;
            }
            con_set_cursor_pos(term,term->cursor_x, term->cursor_y + n1);
            return SUCCESS;

            case 'C': // ESC[#C - Cursor um # Zeilen nach rechts bewegen
              if(have_delimiter || !have_n1 || have_n2)
              {
                return INVALID;
              }
              con_set_cursor_pos(term, term->cursor_x + n1, term->cursor_y);
              return SUCCESS;
                
              case 'D': // ESC[#D - Cursor um # Zeilen nach links bewegen
                if(have_delimiter || !have_n1 || have_n2)
                {
                  return INVALID;
                }
                con_set_cursor_pos(term, term->cursor_x - n1, term->cursor_y);
                return SUCCESS;

                case 'H': // ESC[#;#H - Cursor zur Zeile # und Spalte # bewegen
      case 'f':
        if(!have_n1 || !have_n2)
        {
          return INVALID;
        }
                
        con_set_cursor_pos(term, n1, n2);
        return SUCCESS;

        case 'J': // ESC[2J - Bildschirm leeren
          if(have_delimiter || !have_n1 || have_n2 || n1 != 2)
          {
            return INVALID;
          }
          con_clear_screen(term);
          return SUCCESS;

          case 'K': // ESC[K - Bis Zeile von der Cursor Position zum Ende leeren
            if(have_delimiter || have_n1 || have_n2)
            {
              return INVALID;
            }
                
            con_clear_to(term, term->width);

            return SUCCESS;

            case 'm': // ESC[#m oder ESC[#;#m - Vorder- und Hintergrundfarbe �ndern, Fettschrift, Blinken
              if(!have_n1)
              {
                return INVALID;
              }
                
              if(handle_ansi_formatting_sequence_number(term, n1) != SUCCESS)
              {
                return INVALID;
              }

              if(have_n2)
              {
                if(handle_ansi_formatting_sequence_number(term, n2) != SUCCESS)
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
                term->save_cursor_x = term->cursor_x;
                term->save_cursor_y = term->cursor_y;

                return SUCCESS;

                case 'u': // ESC[u - Cursor-Position wiederherstellen
                  if(have_delimiter || have_n1 || have_n2)
                  {
                    return INVALID;
                  }
                  term->cursor_x = term->save_cursor_x;
                  term->cursor_y = term->save_cursor_y;

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
void con_putc(terminal_t *term, const char c)
{
  static char last_char = 0;

  /*outb(0xe9, c);
  outb(0x3f8, c);
  while ((inb(0x3fd) & 0x20) == 0) asm("nop");*/
    
  switch(c)
  {
    case '\n':
      if (last_char != '\r') {
        con_clear_to(term,term->width);
      }
      term->cursor_x = 0;
      term->cursor_y++;
      break;
        
    case '\r':
      term->cursor_x = 0;
      break;
        
    case '\t':
      term->cursor_x = con_clear_to(term,(term->cursor_x & ~7) + 8);
      break;

    default:
      term->buffer[term->cursor_x + term->cursor_y * term->width] = c | (term->con_color._color << 8);
      term->cursor_x++;
      break;
  }
    
  if(term->cursor_x >= term->width)
  {
    term->cursor_y += term->cursor_x / term->width;
    term->cursor_x %= term->width;
  }
    

  while(term->cursor_y >= term->height)
  {
    scroll_down(term);
  }
  //con_set_hw_cursor();

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
void con_putc_ansi(terminal_t *term, const char c)
{
  ansi_esc_seq_status_t status;
  int i;

  if(c == ASCII_ESC || term->ansi_buf_ofs > 0)
  {
    term->ansi_buf[term->ansi_buf_ofs++] = c;

    status = con_ansi_parse(term, term->ansi_buf, term->ansi_buf_ofs);

    switch(status)
    {
      case NEED_MORE:
        if(term->ansi_buf_ofs <= sizeof(term->ansi_buf))
        {
          break;
        }
                // fall through

      case INVALID:
        for(i = 0; i < term->ansi_buf_ofs; i++)
        {
          con_putc(term,term->ansi_buf[i]);
        }
        term->ansi_buf_ofs = 0;
        break;
            
      case SUCCESS:
        term->ansi_buf_ofs = 0;
        break;
    }
  }
  else
  {
    con_putc(term, c);
  }
}

/**
 * Leert den Puffer, in dem die Zeichen einer m�glichen ANSI-Escape-Code-
 * Sequenz zwischengespeichert werden. Diese Funktion muss aufgerufen werden,
 * wenn sichergestellt werden soll, dass s�mtliche Zeichen, die via 
 * con_putc_ansi ausgegeben werden sollen, tats�chlich ausgegeben werden.
 */
void con_flush_ansi_escape_code_sequence(terminal_t *term)
{
  int i;
    
  if(term->ansi_buf_ofs > 0)
  {
    for(i = 0; i < term->ansi_buf_ofs; i++)
    {
      con_putc(term,term->ansi_buf[i]);
    }
    term->ansi_buf_ofs = 0;
  }
}

/**
 * Gibt eine Zeichenkette auf der Konsole aus. Diese Zeichenkette kann
 * ANSI-Steuercodes enthalten. 
 * Unvollst�ndige und fehlerhafte Sequenzen werden einfach ausgegeben.
 */
void con_puts(terminal_t *term, const char * s)
{
  while(*s)
  {
    con_putc_ansi(term, *s);
    s++;
  }

  con_flush_ansi_escape_code_sequence(term);
}

/**
 * Gibt eine Zeichenkette auf der Konsole aus. Diese Zeichenkette kann
 * ANSI-Steuercodes enthalten. Die Ausgabe terminiert bei Erreichen eines
 * Null-Bytes oder wenn n Bytes ausgewertet wurden.
 * Unvollst�ndige und fehlerhafte Sequenzen werden einfach ausgegeben.
 */
void con_putsn(terminal_t *term, unsigned int n, const char * s)
{
  while(*s && n--)
  {
    con_putc_ansi(term,*s);
    s++;
  }

  con_flush_ansi_escape_code_sequence(term);
}

terminal_t* terminal_getid(int id)
{
    int i;
    terminal_t *term;
    for (i = 0; (term = list_get_element_at(terminals, i)); i++)
    {
        if (term->id == id)
            return term;
    }
    return NULL;
}

size_t terminal_write(lostio_filehandle_t *file, size_t blocksize, size_t blockcount, char *data)
{
    char *buffer = malloc(blocksize * blockcount + 1);
    strncpy(buffer, data, blocksize*blockcount);
    buffer[blocksize * blockcount] = '\0';
    //printf("%s", buffer);
    terminal_t *term = NULL;
    term = terminal_getid(atoi(file->node->name + 4));
    
    if (term == NULL)
    {
        printf("[ VTERM ] No such Terminal! %s\n", file->node->name + 3);
    } 
    else
    {
        con_putsn(term,blocksize * blockcount, data);
        if (curr_terminal = atoi(file->node->name + 4))
            memcpy(textbuffer,term->buffer,term->width * term->height * 2);
        //printf("[ VTERM ] Write %d bytes to Terminal %s\n",blocksize * blockcount, file->node->name);
    }
    free(buffer);
  
  return blocksize * blockcount;
}

terminal_t* terminal_create()
{
    terminal_t *term = NULL;
    term = malloc(sizeof(terminal_t));
    if (term == NULL)
        return NULL;
    term->id = ++terminal_ctr;
    term->width = 80;
    term->height = 25;
    term->buffer = malloc(80 * 25 * 2);
    term->cursor_x = 0; 
    term->cursor_y = 0;
    term->save_cursor_x = 0;
    term->save_cursor_y = 0;
    term->con_color._color = 7;
    int i;
    //for (i = 0; i < term->width * term->height * 2; i++) *((term->buffer) + i) = '\0';
    con_clear_screen(term);
    return term;
}

void terminal_register(terminal_t *term)
{
    if (term == NULL) return;
    char path[9] = "/term";
    itoa(term->id,(char*)&path+5,10);
    vfstree_create_node(path,254,0,NULL,0);
    list_push(terminals,term);
    return;
}

void terminal_change(int id)
{
    p();
    terminal_t* term = terminal_getid(curr_terminal);
    if (term == 0) return;
    char* new_buffer = malloc(term->width * term->height * 2);
    strncpy(new_buffer, term->buffer, term->width * term->height * 2);
    if (term->buffer != textbuffer) free(term->buffer);
    term->buffer = new_buffer;
    term = terminal_getid(id);
    if (term == 0) return;
    strncpy(textbuffer, term->buffer, term->width * term->height *2);
    free(term->buffer);
    term->buffer = textbuffer;
    curr_terminal = id;
    v();
}