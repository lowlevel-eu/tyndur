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

void print(char *string)
{
  const char* hello = "Hello World";
  int i;
  char* videomem = (char*) 0xB8000;
  for (i = 0; i < 11; i++)
  {
    videomem[2*i] = hello[i];
    videomem[2*i + 1] = 7;
  }
  return;
}
