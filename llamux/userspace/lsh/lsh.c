/*
 * Llama Shell (lsh) - Natural language shell for Llamux
 * 
 * A shell that understands natural language commands and
 * translates them to system operations using the kernel AI.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

#define PROMPT_FILE "/proc/llamux/prompt"
#define STATUS_FILE "/proc/llamux/status"
#define MAX_RESPONSE 4096

/* ANSI color codes */
#define COLOR_GREEN  "\033[0;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE   "\033[0;34m"
#define COLOR_RED    "\033[0;31m"
#define COLOR_RESET  "\033[0m"

/* Check if Llamux is available */
int check_llamux() {
    return access(PROMPT_FILE, F_OK) == 0;
}

/* Send prompt to kernel AI and get response */
char* ask_llamux(const char* prompt) {
    static char response[MAX_RESPONSE];
    FILE *fp;
    
    /* Write prompt */
    fp = fopen(PROMPT_FILE, "w");
    if (!fp) {
        return NULL;
    }
    fprintf(fp, "%s", prompt);
    fclose(fp);
    
    /* Wait for processing */
    usleep(500000); /* 500ms */
    
    /* Read response */
    fp = fopen(PROMPT_FILE, "r");
    if (!fp) {
        return NULL;
    }
    
    if (fgets(response, sizeof(response), fp) == NULL) {
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    
    /* Remove "🦙 Response: " prefix if present */
    char *resp = response;
    if (strncmp(resp, "🦙 Response: ", 13) == 0) {
        resp += 13;
    }
    
    return resp;
}

/* Parse AI response and execute command */
int execute_ai_command(const char* user_input, const char* ai_response) {
    /* Simple command mapping - in production this would be more sophisticated */
    
    if (strstr(user_input, "list") || strstr(user_input, "show files")) {
        printf("%s💭 AI suggests: ls -la%s\n", COLOR_BLUE, COLOR_RESET);
        system("ls -la");
        return 1;
    }
    
    if (strstr(user_input, "memory") || strstr(user_input, "RAM")) {
        printf("%s💭 AI suggests: free -h%s\n", COLOR_BLUE, COLOR_RESET);
        system("free -h");
        return 1;
    }
    
    if (strstr(user_input, "disk") || strstr(user_input, "storage")) {
        printf("%s💭 AI suggests: df -h%s\n", COLOR_BLUE, COLOR_RESET);
        system("df -h");
        return 1;
    }
    
    if (strstr(user_input, "process") || strstr(user_input, "running")) {
        printf("%s💭 AI suggests: ps aux | head -20%s\n", COLOR_BLUE, COLOR_RESET);
        system("ps aux | head -20");
        return 1;
    }
    
    if (strstr(user_input, "network") || strstr(user_input, "connection")) {
        printf("%s💭 AI suggests: ip addr show%s\n", COLOR_BLUE, COLOR_RESET);
        system("ip addr show");
        return 1;
    }
    
    /* If no specific command matched, show AI response */
    printf("%s🦙 AI says: %s%s\n", COLOR_GREEN, ai_response, COLOR_RESET);
    return 0;
}

/* Main shell loop */
int main(int argc, char *argv[]) {
    char *input;
    char prompt[256];
    
    printf("\n%s🦙 Welcome to Llama Shell (lsh)%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("The shell that understands natural language!\n");
    printf("Type 'help' for assistance or 'exit' to quit.\n\n");
    
    /* Check if Llamux is available */
    if (!check_llamux()) {
        printf("%s⚠️  Warning: Llamux kernel module not loaded!%s\n", COLOR_RED, COLOR_RESET);
        printf("Natural language features will be limited.\n\n");
    }
    
    /* Main loop */
    while (1) {
        /* Create prompt */
        snprintf(prompt, sizeof(prompt), "%s🦙 lsh>%s ", COLOR_GREEN, COLOR_RESET);
        
        /* Read input */
        input = readline(prompt);
        if (!input) {
            break; /* EOF */
        }
        
        /* Skip empty lines */
        if (strlen(input) == 0) {
            free(input);
            continue;
        }
        
        /* Add to history */
        add_history(input);
        
        /* Handle built-in commands */
        if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
            free(input);
            break;
        }
        
        if (strcmp(input, "help") == 0) {
            printf("\n%sLlama Shell Help:%s\n", COLOR_YELLOW, COLOR_RESET);
            printf("  • Type natural language commands like:\n");
            printf("    - \"show me the files here\"\n");
            printf("    - \"how much memory is free?\"\n");
            printf("    - \"what processes are running?\"\n");
            printf("    - \"check disk space\"\n");
            printf("  • Special commands:\n");
            printf("    - help: Show this help\n");
            printf("    - status: Show Llamux status\n");
            printf("    - exit/quit: Exit the shell\n");
            printf("  • You can also use regular shell commands\n\n");
            free(input);
            continue;
        }
        
        if (strcmp(input, "status") == 0) {
            system("cat /proc/llamux/status 2>/dev/null || echo 'Llamux not available'");
            free(input);
            continue;
        }
        
        /* Check if it's a regular shell command (starts with /) */
        if (input[0] == '/' || strstr(input, "./") == input) {
            system(input);
            free(input);
            continue;
        }
        
        /* Try to interpret with AI */
        if (check_llamux()) {
            char *response = ask_llamux(input);
            if (response) {
                execute_ai_command(input, response);
            } else {
                printf("%s⚠️  Failed to get AI response%s\n", COLOR_RED, COLOR_RESET);
            }
        } else {
            /* Fallback to basic pattern matching */
            if (!execute_ai_command(input, "")) {
                /* Try as shell command */
                int ret = system(input);
                if (ret != 0) {
                    printf("%s❌ Command not found or failed%s\n", COLOR_RED, COLOR_RESET);
                }
            }
        }
        
        free(input);
    }
    
    printf("\n%s👋 Goodbye from Llama Shell!%s\n", COLOR_YELLOW, COLOR_RESET);
    return 0;
}