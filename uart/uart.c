
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define BUF_LEN 1024

void print_hex_array(int len, unsigned char* arr)
{
    int i =0;
    for (; i<len; ++i)
    {
        printf("%c \n", *(arr+i));
    }
    printf("\n");
}


int init_uart()
{
    //-------------------------
    //----- SETUP UART 0 -----
    //-------------------------
    int uart_fs = -1;

    //OPEN THE UART
    uart_fs = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY ); // | O_NDELAY);
    if (uart_fs == -1)
    {
        //ERROR - CAN'T OPEN SERIAL PORT
        printf("Error - Unable to open UART. ?Ensure it is not in use by another application\n");
    }
    else
    {
        printf("Opend UART port OK, number: %d\n", uart_fs);
    }

    //CONFIGURE THE UART
    struct termios options;
    tcgetattr(uart_fs, &options);
    options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart_fs, TCIFLUSH);
    tcsetattr(uart_fs, TCSANOW, &options);

    return uart_fs;
}

void do_transmit(int uart_fd, int send_bytes, unsigned char *p_tx_buffer)
{
    if (uart_fd < 0)
    {
        return;
    }
    //Filestream, bytes to write, number of bytes to write
    int count = write(uart_fd, "hello", 5);


    if (count < 0)
    {
        printf("UART transmit error\n");
    }
}

int checksum(int size, unsigned char* cur_line)
{
    int i =0, checksum=0;
    for (; i<size; ++i)
    {
        if (i!=1 && i!=size-1)
        {
            checksum += *(cur_line+i);
        }
    }
    return ~checksum & 255;
}

void on_receive(int uart_fd)
{
    if (uart_fd < 0)
        return;

    unsigned char rx_buf[BUF_LEN] = {0};
    int rx_length = read(uart_fd, (void*)rx_buf, BUF_LEN-1);
//    if (rx_length < 3)
//    {
        //An error will occur if there are not enough bytes
//        printf("Error occured, read_res: %d\n", rx_length);
//        return;
//    }

    printf("ON RECEIVE: ");
    print_hex_array(rx_length, rx_buf);

    int checkres = checksum(rx_length, rx_buf);
    *(rx_buf+2) = 0xff;
    *(rx_buf+rx_length-1) = checksum(rx_length, rx_buf);

    printf("TO RESPOND: ");
    print_hex_array(rx_length, rx_buf);
    do_transmit(uart_fd, rx_length, rx_buf);
}

void close_uart(int uart_fs)
{
    close(uart_fs);
}

int main()
{
    int uart = init_uart();
    while (1)
    {
        on_receive(uart);
        sleep(0.2);
    }
    close_uart(uart);
    return 0;
}
