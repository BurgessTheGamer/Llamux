/*
 * Proc interface for testing Llamux inference
 * 
 * Provides /proc/llamux/prompt for testing the LLM
 */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

extern struct {
    bool initialized;
    struct mutex lock;
    atomic_t request_pending;
    char *current_prompt;
    char *current_response;
} llama_state;

/* Buffer for prompt input */
static char prompt_buffer[512];

/* Handle prompt write */
static ssize_t llamux_prompt_write(struct file *file, const char __user *buffer,
                                  size_t count, loff_t *pos)
{
    size_t len;
    
    if (!llama_state.initialized) {
        return -ENODEV;
    }
    
    if (atomic_read(&llama_state.request_pending)) {
        return -EBUSY;
    }
    
    len = min(count, sizeof(prompt_buffer) - 1);
    
    if (copy_from_user(prompt_buffer, buffer, len)) {
        return -EFAULT;
    }
    
    prompt_buffer[len] = '\0';
    
    /* Remove trailing newline if present */
    if (len > 0 && prompt_buffer[len - 1] == '\n') {
        prompt_buffer[len - 1] = '\0';
    }
    
    /* Copy prompt to processing buffer */
    mutex_lock(&llama_state.lock);
    strncpy(llama_state.current_prompt, prompt_buffer, 511);
    llama_state.current_prompt[511] = '\0';
    
    /* Clear previous response */
    if (llama_state.current_response) {
        llama_state.current_response[0] = '\0';
    }
    
    /* Signal inference thread */
    atomic_set(&llama_state.request_pending, 1);
    mutex_unlock(&llama_state.lock);
    
    pr_info("🦙 Llamux: Received prompt: %s\n", prompt_buffer);
    
    return count;
}

/* Handle prompt read - shows last response */
static int llamux_prompt_show(struct seq_file *m, void *v)
{
    if (atomic_read(&llama_state.request_pending)) {
        seq_printf(m, "🦙 Processing...\n");
    } else if (llama_state.current_response && 
               strlen(llama_state.current_response) > 0) {
        seq_printf(m, "🦙 Response: %s\n", llama_state.current_response);
    } else {
        seq_printf(m, "🦙 Ready for prompt. Write to this file to test inference.\n");
        seq_printf(m, "Example: echo \"What is Linux?\" > /proc/llamux/prompt\n");
    }
    
    return 0;
}

static int llamux_prompt_open(struct inode *inode, struct file *file)
{
    return single_open(file, llamux_prompt_show, NULL);
}

const struct proc_ops llamux_prompt_fops = {
    .proc_open = llamux_prompt_open,
    .proc_read = seq_read,
    .proc_write = llamux_prompt_write,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

/* Create prompt interface */
int llamux_create_prompt_interface(struct proc_dir_entry *parent)
{
    struct proc_dir_entry *entry;
    
    entry = proc_create("prompt", 0666, parent, &llamux_prompt_fops);
    if (!entry) {
        pr_err("🦙 Llamux: Failed to create /proc/llamux/prompt\n");
        return -ENOMEM;
    }
    
    pr_info("🦙 Llamux: Created /proc/llamux/prompt interface\n");
    return 0;
}