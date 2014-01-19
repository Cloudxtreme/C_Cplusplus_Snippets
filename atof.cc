// rough function

float strToFloat (char * str) {
  // check
  if (!str) {
    return 0.0;
  }

  bool isNegative = false;
  bool isDec = false;

  float integer = 0.0;
  float decimal = 0.0;
  float dec_power = 0.1;

  char * cur = 0;
  char charByte = ‘0’;

  if (str[0] == ‘-’) {
    isNegative = true;
    cur = str + 1;
  } else {
    isNegative = false;
    cur = str;
  }

  while (*cur != ‘\0’) {
    charByte = *cur;

    if (isDec) {
      // dec
      if (charByte >= ‘0’ && charByte <= ‘9’) {
        decimal += (charByte - ‘0’) * dec_power;
        dec_power *= 0.1;
      } // else {
        // get the result
        // result: -(integer + decimal) | (integer + decimal)
      // return (isNegative? -(integer + decimal) : (integer + decimal);
      // }
    } else {
      // integer
      if (charByte >= ‘0’ && charByte <= ‘9’) {
        integer = integer * 10 + (charByte - ‘0’);
      } else if (charByte == ‘.’) {
        isDec = true;
      } //else {
        // result
        // return (isNegative? : -(integer) : integer);
      //}
    }

    cur++;
  }
  return (isNegative? -(integer + decimal) : (integer + decimal));
}
