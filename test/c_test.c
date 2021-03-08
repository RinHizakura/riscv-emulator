int fib(int n);
void uart();
void echo();

int main()
{
    echo();
    uart();
    return fib(10);
}

int fib(int n)
{
    if (n == 0)
        return 0;
    if (n == 1)
        return 1;
    return (fib(n - 1) + fib(n - 2));
}

void uart()
{
    volatile char *uart = (volatile char *) 0x10000000;
    uart[0] = 'H';
    uart[0] = 'e';
    uart[0] = 'l';
    uart[0] = 'l';
    uart[0] = 'o';
    uart[0] = ',';
    uart[0] = ' ';
    uart[0] = 'w';
    uart[0] = 'o';
    uart[0] = 'r';
    uart[0] = 'l';
    uart[0] = 'd';
    uart[0] = '!';
    uart[0] = '\n';
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
