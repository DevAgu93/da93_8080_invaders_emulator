//Variable arg lists.
//these macros are used to pass an unspecified amount of arguments
//to a function and reading them from the stack memory.
#define _INC_STDARG
typedef char* va_list;

//round the size of the parameter to the nearest integer boundary
#define _INTSIZEOF(n)          ((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))
#define _ADDRESSOF(src) (&(src))


//make args point to where the stack would start
//src is used to know where the next variables will be
//on the stack from its direction
#define _crt_va_start(args, src) ((void)(args = (va_list)_ADDRESSOF(src) + _INTSIZEOF(src)))
//advance args by the size of type rounded to the nearest integer boundary and return
//the variable.
#define _crt_va_arg(args, type) (*(type*)((args += _INTSIZEOF(type)) - _INTSIZEOF(type)))
#define _crt_va_end(args) ((void)(args = (va_list)0))

#define va_start_m _crt_va_start
#define va_arg_m   _crt_va_arg
#define va_end_m   _crt_va_end
#define va_copy(destination, source) ((destination) = (source))

#define char_is_num(ch) ((ch) >= '0' && (ch) <= '9')


static unsigned int
char_to_u32(char ch)
{
	unsigned int result = ch - '0';

	if(ch < '0' || ch > '9')
	{
		result = 0;
	}

	return(result);
}

static unsigned int
u32_from_string_i(unsigned char *text, int s_i, int e_i)
{
	char *at = text + s_i;
	unsigned int number = 0;
	int i = s_i;

	while(char_is_num(*at) && *at != '\0' && i <= e_i)
	{
		//Check multiply overflow, because it always gets multiplied by 10
		const unsigned int u32_max = 4294967295;
	   if(number && (10 > u32_max / number))
	   {
		   number = u32_max;
		   break;
	   }
	   //because of how numbers are encoded
	   unsigned int digit = (*at) - '0';
	   number *= 10;
	   number += digit;
	   i++;
	   at++;
	}
	unsigned int result = number;
	return(result);

}

static unsigned int
u32_from_string(unsigned char *text)
{
	char *at = text;
	unsigned int number = 0;
	unsigned int i = 0;

	//while((ch = text[i]) != '\0')
	while(char_is_num(*at) && *at != '\0')
	{
		//Check multiply overflow, because it always gets multiplied by 10
		const unsigned int u32_max = 4294967295;
	   if(number && (10 > u32_max / number))
	   {
		   number = u32_max;
		   break;
	   }
	   //because of how numbers are encoded
	   unsigned int digit = (*at) - '0';
	   number *= 10;
	   number += digit;
	   i++;
	   at++;
	}
	unsigned int result = number;
	return(result);

}

inline static void
fmt_push_char(char* buffer, unsigned short *at, char c)
{
    buffer[*at] = c;
    (*at)++;
}

inline static void 
fmt_u32_to_ascii(char *buffer, unsigned short *at, unsigned int value)
{
   char c = 0;

   //copy the current buffer location
   unsigned short start = *at;

   //push the first number
   c = (value % 10) + '0';
   fmt_push_char(buffer, at, c);

   //while it's divisible by 10
   while((value = value / 10))
   {
     c = (value % 10) + '0';
     fmt_push_char(buffer, at, c);
   }
   unsigned short end = *at - 1;
   while(end > start)
   {
      char digit = buffer[start];
      buffer[start++] = buffer[end];
      buffer[end--] = digit;
   }
   
}
inline static void 
i32_to_ascii(char *buffer, unsigned short *at, int value)
{
   char c = 0;

   unsigned short start = *at;
   c = (value % 10) + '0';
   fmt_push_char(buffer, at, c);

   while((value = value / 10))
   {
     c = (value % 10) + '0';
     fmt_push_char(buffer, at, c);
   }

   
   unsigned short end = *at - 1;
   while(end > start)
   {
      char digit = buffer[start];
      buffer[start++] = buffer[end];
      buffer[end--] = digit;
   }
   
}

//=====================================
static int 
format_text_list(char* buffer, unsigned int buffer_size, char* format, va_list args)
{
    char c = format[0];

    unsigned short text_formated_at = 0;

	int llFormat = 0;
    for(unsigned int i = 0;
            i < buffer_size;
            i++)
    {
        (c = format[i]);

		//use a default precision
       int precision = 3;
       if(c == '%')
       {
           i++;

		   //default for hex
		   int param_n = 4;

		   //TODO Add specifier format
#if 0
		   int param_amount = 0;
		   //detect number inside specifier
		   if(format[i] >= '0' && format[i] <= '9')
		   {

			   int start = i;
			   char ch;

			   do
			   {
				   ch = format[i++];
			   } while(ch >= '0' && ch <= '9');

			   param_n = u32_from_string_i(format, start, i - 1);
		   }
#endif
		   unsigned char ch = format[i];
		   while(ch >= '0' && ch <= '9')
		   {
			   ch = format[++i];
		   }

		   unsigned int processing_format = 1;
		   while(processing_format)
		   {


               switch(format[i])
               {
		           //Precision EJ: %.9f shows up to 9 numbers
		           case '.':
		        	   {
		        		   unsigned int digitCount = 0;

		        		   unsigned char digitBuffer[24] = {0};

		        		   c = format[++i];
		        		   while(c >= '0' && c <= '9')
		        		   {

		        			   digitBuffer[digitCount] = c;
		        			   digitCount++;
		        			   i++;
		        			   c = format[i];
		        		   }

		        		   //Precision has sign!
		        		   precision = u32_from_string(digitBuffer);

		        		   if(precision > 9)
		        		   {
		        			   precision = 9;
		        		   }

		        	   }break;
		           //Specifiers
		          case 'l':
		        	   {
		        			//Dont break, continue
		        		   llFormat = 1;
		        		   while(format[++i] == 'l');
		        		   processing_format = 0;
		        	   }break;
                   case 's':
                       {

                          char *l = va_arg_m(args, char *); 
						  if(l)
						  {
							  while(l[0])
							  {
								  fmt_push_char(buffer, &text_formated_at, l[0]);
								  l++;
								  //print
							  }
						  }
		        		   processing_format = 0;
                       }break;
                   case 'c':
                       {

                          char l = va_arg_m(args, int); 
                          fmt_push_char(buffer, &text_formated_at, l);
		        		   processing_format = 0;
                       }break;
				   case 'x':
					   {
                           unsigned int value0 = (unsigned int)va_arg_m(args, unsigned long long);
                           unsigned int value1 = value0; 

						//   fmt_push_char(buffer, &text_formated_at, '0');
						//   fmt_push_char(buffer, &text_formated_at, 'x');

						   if(!value0)
						   {
							   int total_n = 1;
							   while(total_n < param_n)
							   {
								   total_n++;
								   fmt_push_char(buffer, &text_formated_at, '0');
							   }

							   fmt_push_char(buffer, &text_formated_at, '0');
						   }
						   else
						   {

							   char hex_buff[16];
							   int hex_c = 0;

							   while(value0)
							   {
								   value1 /= 16;
								   unsigned int remainder = value1;
								   remainder *= 16;
								   //remainder
								   remainder = value0 - remainder;

								   value0 = value1;

								   //						   A remainder is always less than the divisor.

								   if(remainder >= 16 || hex_c >= 16)
								   {
									   fmt_push_char(buffer, &text_formated_at, 'E');
									   fmt_push_char(buffer, &text_formated_at, 'r');
									   fmt_push_char(buffer, &text_formated_at, 'r');
								   }
								   else
								   {
									   const char hex_vals[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
									   //								   fmt_push_char(buffer, &text_formated_at, hex_vals[remainder]);
									   hex_buff[hex_c++] = hex_vals[remainder];
								   }
							   }

							   int total_n = hex_c;
							   while(total_n < param_n)
							   {
								   total_n++;
								   fmt_push_char(buffer, &text_formated_at, '0');
							   }

							   while(hex_c--)
							   {
								   fmt_push_char(buffer, &text_formated_at, hex_buff[hex_c]);
							   }
						   }



//                           fmt_u32_to_ascii(buffer ,&text_formated_at, value);
		        		   processing_format = 0;
					   }break;
                  case 'u':
                       {
						   //UNSIGNED lONG LONG SHOULDN'T BE CORRECT because it depends on the platform. 
                           unsigned int value = (unsigned int)va_arg_m(args, unsigned long long);

                           fmt_u32_to_ascii(buffer ,&text_formated_at, value);
		        		   processing_format = 0;
                       }break;
                  case 'd':
                       {
                           int value = (int)va_arg_m(args, long long);

                           if(value < 0)
                           {
                              fmt_push_char(buffer, &text_formated_at, '-');
                              value = -value;
                           }
                           i32_to_ascii(buffer , &text_formated_at, value);
		        		   processing_format = 0;

                       }break;
                  case 'f':
                       {
                           float value = (float)va_arg_m(args, double);
                           if(value < 0)
                           {
                               fmt_push_char(buffer, &text_formated_at, '-');
                               value = -value;
                           }
                           int valueInteger = (int)value;
		        		   //Get rid of decimals
                           value           -= valueInteger;
		        		   //Push the whole number
                           i32_to_ascii(buffer , &text_formated_at, valueInteger);


                           fmt_push_char(buffer, &text_formated_at, '.');
                           for(int p = 0;
                                   p < precision;
                                   p++)
                           {
                              value *= 10;
		        			  //Add the '0' char to place the value on the correct place
                              unsigned char value8 = ((unsigned int)value % 10) + '0'; 
                              fmt_push_char(buffer, &text_formated_at, value8);
                           }
		        		   processing_format = 0;

                       }break;
		          case 'n':
		        	   {
		        		   //Not implemented yet
		        		   processing_format = 0;

		        	   }break;
				  case '\0':
					   {
						   fmt_push_char(buffer, &text_formated_at, '\0');
		        		   processing_format = 0;
					   }break;
				  default:
					   {
						   
						   //Assert(0);
						   processing_format = 0;
					   }
                   
               }
		   }
       }
       else if(c == '\0')
       {
           fmt_push_char(buffer, &text_formated_at, c);
           break;
       }
       else 
       {
           fmt_push_char(buffer, &text_formated_at, c);
       }

    }

    unsigned char *lastc = buffer;
    unsigned int text_size = 1;

    while(*lastc != '\0')
    {
        lastc++;
        text_size++;
    }
    return(text_size);
}

static int
format_text(char* buffer, unsigned int buffer_size, char* format, ...)
{
    va_list args;
    va_start_m(args, format);
    unsigned int text_size = format_text_list(buffer, buffer_size, format, args);
    va_end_m(args);
    return text_size;
}
