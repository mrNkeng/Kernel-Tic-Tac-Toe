// Include necessary header files
#include <linux/init.h> // defines initializations for macros that are called when module is loaded and unloaded.
#include <linux/kernel.h> // contains core kernel definitions
#include <linux/fs.h> // provide structures for creating a character device.
#include <linux/uaccess.h> // safe copying of data from kernel to user space.
#include <linux/miscdevice.h> //Alternative to full character device and doesn't need manual management of device numbers
#include <linux/random.h> // generate random numbers
#include <linux/module.h> // has definitions and function for kernel module development

MODULE_LICENSE("GPL"); //license under whick the kernel module is distributed
MODULE_AUTHOR("Aminkeng Nkeng");
MODULE_DESCRIPTION("This Kernel Module will inspire you");
MODULE_VERSION("0.1"); // the version number

// Array of inspirational quotes
static const char *quotes[] = {
    "'We must let go of the life we have planned, so as to accept the one that is waiting for us.' - Joseph Campbell\n",
    "'You can have everything in life you want, if you will just help other people get what they want.' - Zig Ziglar\n",
    "'If there is no struggle, there is no progress.' - Frederick Douglass\n",
    "'The best preparation for tomorrow is doing your best today.'- H. Jackson Brown, Jr.\n",
    "'It is never too late to be what you might have been.' - George Eliot\n",
    "'Strength and growth come only through continuous effort and struggle.' - Napoleon Hill\n",
    "'If you fell down yesterday, stand up today.' - H. G. Wells\n",
    "'No act of kindness, no matter how small, is ever wasted' - Aesop\n",
    "'The most common way people give up their power is by thinking they don't have any.' - Alice Walker\n",
    "'Failure will never overtake me if my determination to succeed is strong enough.' - Og Mandino\n",
    "'One way to get the most out of life is to look upon it as an adventure.' - William Feather\n",
    "'One's destination is never a place but rather a new way of looking at things.' - Henry Miller\n",
    "'Success is the sum of small efforts - repeated day in and day out.' - Robert Collier\n",
    "'Circumstances do not make the man, they reveal him.' - James Allen\n",
    "'Life isn't a matter of milestones, but of moments.' - Rose Kennedy\n",
    "'Never give up, for that is just the place and time that the tide will turn.' - Harriet Beecher Stowe\n",
    "'I believe that the most meaningful way to succeed is to help other people succeed.' - Adam Grant\n",
    "'There is no innovation and creativity without failure.' - Brene Brown\n",
    "'Success isn't a result of spontaneous combustion. You must set yourself on fire.' - Arnold H. Glasow\n",
    "'There are two ways of spreading light: to be the candle or the mirror that reflects it.' - Edith Wharton\n"
};

// Read function for the character device driver
// return the number of bytes read or an Error
static ssize_t insp_read(struct file *file, char __user *buf, size_t count, loff_t *pos) {
    //variables to store the randomly selected quote and its length
    unsigned int quote_index;
    const char *quote;
    size_t quote_len;

    // Generate a random index for quote selection
    //select the quote at that particukar index and calculate its length
    get_random_bytes(&quote_index, sizeof(quote_index));
    quote_index %= ARRAY_SIZE(quotes);
    quote = quotes[quote_index];
    quote_len = strlen(quote);

    // Check if the entire quote has been read, and return 0 if so
    if (*pos >= quote_len){
        return 0;
    }
    // Ensure we don't read beyond the quote length
    if (count > quote_len - *pos){
        count = quote_len - *pos;
    }
    // Copy the quote data to user space buffer, return an error if unsuccessful
    if (copy_to_user(buf, quote + *pos, count)){
        return -EFAULT;
    }

    // Update the file position and return the number of bytes read
    *pos += count;
    return count;
}

// Write function for the character device
//called when user attempts to write to the file
static ssize_t insp_write(struct file *file, const char __user *buf, size_t count, loff_t *pos) {
    // Deny write access to the device with a "Permission Denied" error
    return -EPERM; //permission denied
}

// definesFile operations for the character device
//sets read and write callbacks to the functions created above
static const struct file_operations inspiration_fops = {
    .read = insp_read,
    .write = insp_write,
    .owner = THIS_MODULE, // the ownership of the structure is associated with the module itself so resources are not released prematurely while the structure is still in use
};

// Define the misc device for the character device
static struct miscdevice inspiration_misc_device = {
    // sets the minor number of the charaacter device driver to be allocated dynamiclly
    .minor = MISC_DYNAMIC_MINOR,
    .name = "inspiration",
    .fops = &inspiration_fops,
    //sets character device name and then set file operation for character device
};

// Initialization function for the kernel module
static int __init inspiration_init(void) {
    int load;
    // Register the misc device
    load = misc_register(&inspiration_misc_device);
    if (load)
        pr_err("Failed to register /dev/inspiration\n");
    return load;
}

// Exit function for the kernel module
static void __exit inspiration_exit(void) {
    // Unregister the misc device
    misc_deregister(&inspiration_misc_device);
}

// Specify and register the initialization and exit functions
module_init(inspiration_init);
module_exit(inspiration_exit);