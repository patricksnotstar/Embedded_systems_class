#include <linux/module.h>
#include <linux/miscdevice.h> // for misc-driver calls.
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/kfifo.h>
#include <linux/leds.h>
#include <linux/ctype.h>

// #error Are we building this file?

#define MY_DEVICE_FILE "morse-code"

// Why is sus boi so sus??? Because he loves spiderman Nick, he can get into unique positions

//KFIFO
#define FIFO_SIZE 512 // Must be a power of 2.
static DECLARE_KFIFO(echo_fifo, char, FIFO_SIZE);

// LED
DEFINE_LED_TRIGGER(led_trigger);

// HEX STUFF
#define CAPITAL_A 65 // A
#define CAPITAL_Z 90 // Z
#define LOWER_A 97   // a
#define LOWER_Z 122  // z
#define MASK 0x8000

// how many milliseconds dots/dashes last
#define DOT 200
#define DASH 600
// time between each letter
#define LETTERSPACE 600
// time between each word
#define WORDSPACE 1400
#define NOSLEEP 0

static unsigned short morsecode_codes[] = {
    0xB800, // A 1011 1
    0xEA80, // B 1110 1010 1
    0xEBA0, // C 1110 1011 101
    0xEA00, // D 1110 101
    0x8000, // E 1
    0xAE80, // F 1010 1110 1
    0xEE80, // G 1110 1110 1
    0xAA00, // H 1010 101
    0xA000, // I 101
    0xBBB8, // J 1011 1011 1011 1
    0xEB80, // K 1110 1011 1
    0xBA80, // L 1011 1010 1
    0xEE00, // M 1110 111
    0xE800, // N 1110 1
    0xEEE0, // O 1110 1110 111
    0xBBA0, // P 1011 1011 101
    0xEEB8, // Q 1110 1110 1011 1
    0xBA00, // R 1011 101
    0xA800, // S 1010 1
    0xE000, // T 111
    0xAE00, // U 1010 111
    0xAB80, // V 1010 1011 1
    0xBB80, // W 1011 1011 1
    0xEAE0, // X 1110 1010 111
    0xEBB8, // Y 1110 1011 1011 1
    0xEEA0  // Z 1110 1110 101
};

static void led_register(void)
{
    // Setup the trigger's name:
    led_trigger_register_simple("morse-code", &led_trigger);
}

static void led_unregister(void)
{
    // Cleanup
    led_trigger_unregister_simple(led_trigger);
}

static ssize_t read(struct file *file,
                    char *buf, size_t count, loff_t *ppos)
{
    // Pull all available data from fifo into user buffer
    int num_bytes_read = 0;
    if (kfifo_to_user(&echo_fifo, buf, count, &num_bytes_read))
    {
        return -EFAULT;
    }

    return num_bytes_read; // # bytes actually read.
}

static unsigned short get_hex(char c)
{
    if (c == ' ')
    {
        return 0;
    }

    // subtract 65 from the chars value to get its index in the struct
    // uppercase letters
    else if (c >= CAPITAL_A && c <= CAPITAL_Z)
    {
        return morsecode_codes[c - CAPITAL_A];
    }
    // lowercase letters
    else if (c >= LOWER_A && c <= LOWER_Z)
    {
        return morsecode_codes[c - LOWER_A];
    }
    // means it is some other special char such as !
    return -1;
}

// add character to queue and check it was successful
static int add_to_queue(char c)
{
    if (!kfifo_put(&echo_fifo, c))
    {
        return -EFAULT;
    }

    return 0;
}

static void flash_led(bool mode, int sleeptime)
{
    // if mode is true (i.e. 1)
    // turn light on
    if (mode)
    {
        led_trigger_event(led_trigger, LED_ON);
    }
    // if mode is false (i.e 0)
    // turn light off
    else
    {
        led_trigger_event(led_trigger, LED_OFF);
    }
    msleep(sleeptime);
}

static void handle_hex(unsigned short hex_value)
{
    int i = 0;
    int ones_in_a_row_counter = 0;
    unsigned short shifted_mask;
    // if its not a space or a letter just skip it
    if (hex_value == -1)
    {
        return;
    }
    // otherwise it's a letter, meaning we need to flash for each bit
    else
    {
        // need to loop through 16 bits of each binary character from left to right
        // and keep track of appearances of 10 (dot) and 1110 (dash)
        // start loop with 1000 0000 0000 0000
        bool firstFlash = true;
        for (i = 0; i < 16; i++)
        {
            shifted_mask = MASK >> i;

            // AND the hex value and mask together to see if there is a one in the ith position
            // so check if it is equal to the shifted bit mask
            if ((hex_value & shifted_mask) == shifted_mask)
            {
                // means we encountered a 1
                ones_in_a_row_counter++;
            }
            // means we encountered a zero
            else
            {
                // if we are seeing a zero after 3 1's in a row
                // that means its a dash
                if (ones_in_a_row_counter == 3)
                {
                    if (firstFlash)
                    {
                        add_to_queue('-');
                        flash_led(true, DASH);
                        flash_led(false, NOSLEEP);
                        firstFlash = false;
                    }
                    else
                    {
                        flash_led(false, DOT);
                        add_to_queue('-');
                        // turn on LED
                        flash_led(true, DASH);
                        flash_led(false, NOSLEEP);
                    }
                    ones_in_a_row_counter = 0;
                }
                // if ones in a row counter is 1 that means we are seeing 10
                // which is a dot
                else if (ones_in_a_row_counter == 1)
                {
                    if (firstFlash)
                    {
                        // put a dot into the kfifo
                        add_to_queue('.');
                        // turn on LED
                        flash_led(true, DOT);
                        flash_led(false, NOSLEEP);
                        firstFlash = false;
                    }
                    else
                    {
                        flash_led(false, DOT);
                        add_to_queue('.');
                        flash_led(true, DOT);
                        flash_led(false, NOSLEEP);
                    }
                    ones_in_a_row_counter = 0;
                }
            }
        }
    }
}

static ssize_t write(struct file *file,
                     const char *buff, size_t count, loff_t *ppos)
{
    int buff_idx = 0;
    unsigned short hexVal;
    char prevChar = 0;
    bool firstWord = true;
    int i = 0;

    printk(KERN_INFO "morsecode driver: In my_write()\n");

    for (buff_idx = 0; buff_idx < count; buff_idx++)
    {
        char ch;
        // Get the character
        if (copy_from_user(&ch, &buff[buff_idx], sizeof(ch)))
        {
            return -EFAULT;
        }

        if (isalpha(ch))
        {
            if (firstWord)
            {
                firstWord = false;
            }
            else
            {
                if (prevChar == ' ')
                {
                    for (i = 0; i < 3; i++)
                    {
                        add_to_queue(' ');
                    }
                    msleep(WORDSPACE);
                }
            }
            hexVal = get_hex(ch);
            if (hexVal != -1)
            {
                if (isalpha(prevChar))
                {
                    // need to sleep for 600ms in between each letter
                    msleep(LETTERSPACE);
                    // add a space to output
                    add_to_queue(' ');
                }
                handle_hex(hexVal);
            }
            prevChar = ch;
        }
        else if (ch == ' ')
        {
            prevChar = ch;
        }
    }
    add_to_queue('\n');
    // increment position
    *ppos += count;
    // Return # bytes actually written.
    return count;
}

/******************************************************
 * Misc support
 ******************************************************/
// Callbacks:  (structure defined in /linux/fs.h)
struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .read = read,
    .write = write,
};

// Character Device info for the Kernel:
static struct miscdevice my_miscdevice = {
    .minor = MISC_DYNAMIC_MINOR, // Let the system assign one.
    .name = MY_DEVICE_FILE,      // /dev/.... file.
    .fops = &my_fops             // Callback functions.
};

static int __init mydriver_init(void)
{
    printk(KERN_INFO "----> Morsecode driver init()\n");
    // Register as a misc driver:
    INIT_KFIFO(echo_fifo);
    // LED:
    led_register();

    return misc_register(&my_miscdevice);
}

static void __exit mydriver_exit(void)
{
    printk(KERN_INFO "<---- Morsecode driver exit().\n");
    // Unregister misc driver
    misc_deregister(&my_miscdevice);
    led_unregister();
}

// Link our init/exit functions into the kernel's code.
module_init(mydriver_init);
module_exit(mydriver_exit);
// Information about this module:

MODULE_AUTHOR("Shayna Grose & Patrick Nguyen");
MODULE_DESCRIPTION("Morse code driver");
MODULE_LICENSE("GPL");