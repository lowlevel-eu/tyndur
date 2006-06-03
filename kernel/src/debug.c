/* Debug-Funktionen und Helferlein */

/* Int2Str , Wandelt signed ints in Strings um */
char* int2str(signed int value,char *result)
{
  int negative = 0;
  int counter = 10;
  if (value < 0)
  {
    value = value * -1;
    negative = 1;
  }
  *(result+counter) = '\0';
  counter--;
  while (value)
  {
    *(result + counter) = value%10+'0'; // + 48 eigtl, 48 = Ascii 0
    value = value / 10;
    counter--;
  }
  if (negative)
  {
    *(result+counter) = '-'; // Eigtl, 45 = Ascii -
    counter--;
  }
  return result+counter+1;
}

void print(char *string,const int len)
{
  static int cursor = 0;
  int i;
  
  char* videomem = (char*) 0xB8000;
  for (i = 0; i < len; i++)
  {
    videomem[2*cursor] = string[i];
    videomem[2*cursor + 1] = 7;
    cursor++;
  }

  cursor += 80 - (cursor % 80);
  
  return;
}
