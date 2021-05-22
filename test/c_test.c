int fib(int n);
void uart_hex(int d);
void echo();
void uart_send(char c);
void uart_send_string(char *str);

int main()
{
    // echo();
    int ans = fib(20);
    uart_send('a');
    uart_send('n');
    uart_send('s');
    uart_send(':');

    uart_hex(ans);
    uart_send('\n');

    return 0;
}

int fib(int n)
{
    if (n == 0)
        return 0;
    if (n == 1)
        return 1;
    return (fib(n - 1) + fib(n - 2));
}

void uart_hex(int d)
{
    volatile char *uart = (volatile char *) 0x10000000;
    unsigned int n;
    int c;
    for (c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        uart[0] = n;
    }
}

void uart_send(char c)
{
    volatile char *uart = (volatile char *) 0x10000000;
    uart[0] = c;
}

void uart_send_string(char *str)
{
    volatile char *uart = (volatile char *) 0x10000000;
    for (int i = 0; str[i] != '\0'; i++) {
        uart[0] = str[i];
    }
}

void echo()
{
    while (1) {
        volatile char *uart = (volatile char *) 0x10000000;
        while ((uart[5] & 0x01) == 0)
            ;
        char c = uart[0];
        if ('a' <= c && c <= 'z') {
            c = c + 'A' - 'a';
        }
        uart[0] = c;

        if (c == 'X')
            break;
    }
}
