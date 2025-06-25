#include <wut.h>
#include <coreinit/screen.h>
#include <coreinit/thread.h>
#include <coreinit/memory.h>
#include <coreinit/memdefaultheap.h>
#include <vpad/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_INPUT 12

int password[] = {
    VPAD_BUTTON_RIGHT, VPAD_BUTTON_DOWN, VPAD_BUTTON_UP, VPAD_BUTTON_UP, 
    VPAD_BUTTON_LEFT, VPAD_BUTTON_DOWN, VPAD_BUTTON_DOWN, VPAD_BUTTON_UP, VPAD_BUTTON_DOWN
};
int password_len = 9;
int input_sequence[MAX_INPUT];
int input_index = 0;

bool validate_password() {
    if (input_index != password_len) return false;
    for (int i = 0; i < password_len; i++) {
        if (input_sequence[i] != password[i]) return false;
    }
    return true;
}

void reset_input() {
    input_index = 0;
    memset(input_sequence, 0, sizeof(input_sequence));
}

int main(int argc, char **argv) {
    // Initialize screen
    OSScreenInit();
    size_t tvBufferSize = OSScreenGetBufferSizeEx(SCREEN_TV);
    size_t drcBufferSize = OSScreenGetBufferSizeEx(SCREEN_DRC);
    
    void *tvBuffer = MEMAllocFromDefaultHeapEx(tvBufferSize, 0x100);
    void *drcBuffer = MEMAllocFromDefaultHeapEx(drcBufferSize, 0x100);
    
    // If allocation fails, just continue (don't block boot)
    if (!tvBuffer || !drcBuffer) {
        if (tvBuffer) MEMFreeToDefaultHeap(tvBuffer);
        if (drcBuffer) MEMFreeToDefaultHeap(drcBuffer);
        return 0; // Continue boot sequence
    }
    
    OSScreenSetBufferEx(SCREEN_TV, tvBuffer);
    OSScreenSetBufferEx(SCREEN_DRC, drcBuffer);
    OSScreenEnableEx(SCREEN_TV, 1);
    OSScreenEnableEx(SCREEN_DRC, 1);
    
    VPADInit();
    VPADStatus status;
    VPADReadError error;
    
    // Shorter timeout for setup module - 30 seconds
    uint32_t timeout_counter = 0;
    const uint32_t TIMEOUT_LIMIT = 30 * 1000 / 50; // 30 seconds
    
    bool password_verified = false;
    
    while (!password_verified) {
        OSScreenClearBufferEx(SCREEN_TV, 0x000000);
        OSScreenClearBufferEx(SCREEN_DRC, 0x000000);
        
        OSScreenPutFontEx(SCREEN_TV, 0, 0, "TIRAMISU SECURITY MODULE");
        OSScreenPutFontEx(SCREEN_DRC, 0, 0, "TIRAMISU SECURITY MODULE");
        OSScreenPutFontEx(SCREEN_TV, 0, 1, "Enter password to continue boot");
        OSScreenPutFontEx(SCREEN_DRC, 0, 1, "Enter password to continue boot");
        
        char progress[64] = "Sequence: ";
        for (int i = 0; i < password_len; i++) {
            if (i < input_index) {
                strcat(progress, "*");
            } else {
                strcat(progress, "_");
            }
            if (i < password_len - 1) strcat(progress, " ");
        }
        OSScreenPutFontEx(SCREEN_TV, 0, 3, progress);
        OSScreenPutFontEx(SCREEN_DRC, 0, 3, progress);
        
        OSScreenPutFontEx(SCREEN_TV, 0, 5, "D-Pad: Enter sequence");
        OSScreenPutFontEx(SCREEN_DRC, 0, 5, "D-Pad: Enter sequence");
        OSScreenPutFontEx(SCREEN_TV, 0, 6, "+: Submit   HOME: Reset");
        OSScreenPutFontEx(SCREEN_DRC, 0, 6, "+: Submit   HOME: Reset");
        
        // Show timeout
        int remaining_seconds = (TIMEOUT_LIMIT - timeout_counter) * 50 / 1000;
        char timeout_msg[64];
        sprintf(timeout_msg, "Auto-continue in %d seconds", remaining_seconds);
        OSScreenPutFontEx(SCREEN_TV, 0, 8, timeout_msg);
        OSScreenPutFontEx(SCREEN_DRC, 0, 8, timeout_msg);
        
        OSScreenFlipBuffersEx(SCREEN_TV);
        OSScreenFlipBuffersEx(SCREEN_DRC);
        
        VPADRead(VPAD_CHAN_0, &status, 1, &error);
        
        if (error == VPAD_READ_SUCCESS) {
            uint32_t pressed = status.trigger;
            
            // Reset timeout on any input
            if (pressed != 0) {
                timeout_counter = 0;
            }
            
            if (input_index < password_len) {
                if (pressed & VPAD_BUTTON_UP) {
                    input_sequence[input_index++] = VPAD_BUTTON_UP;
                }
                else if (pressed & VPAD_BUTTON_DOWN) {
                    input_sequence[input_index++] = VPAD_BUTTON_DOWN;
                }
                else if (pressed & VPAD_BUTTON_LEFT) {
                    input_sequence[input_index++] = VPAD_BUTTON_LEFT;
                }
                else if (pressed & VPAD_BUTTON_RIGHT) {
                    input_sequence[input_index++] = VPAD_BUTTON_RIGHT;
                }
            }
            
            if (pressed & VPAD_BUTTON_PLUS) {
                if (validate_password()) {
                    // Show success briefly
                    OSScreenClearBufferEx(SCREEN_TV, 0x00FF00);
                    OSScreenClearBufferEx(SCREEN_DRC, 0x00FF00);
                    OSScreenPutFontEx(SCREEN_TV, 0, 8, "PASSWORD ACCEPTED");
                    OSScreenPutFontEx(SCREEN_DRC, 0, 8, "PASSWORD ACCEPTED");
                    OSScreenPutFontEx(SCREEN_TV, 0, 10, "Continuing Tiramisu boot...");
                    OSScreenPutFontEx(SCREEN_DRC, 0, 10, "Continuing Tiramisu boot...");
                    OSScreenFlipBuffersEx(SCREEN_TV);
                    OSScreenFlipBuffersEx(SCREEN_DRC);
                    OSSleepTicks(OSMillisecondsToTicks(2000));
                    
                    password_verified = true;
                } else {
                    // Show failure and reset
                    OSScreenClearBufferEx(SCREEN_TV, 0xFF0000);
                    OSScreenClearBufferEx(SCREEN_DRC, 0xFF0000);
                    OSScreenPutFontEx(SCREEN_TV, 0, 8, "INCORRECT PASSWORD");
                    OSScreenPutFontEx(SCREEN_DRC, 0, 8, "INCORRECT PASSWORD");
                    OSScreenPutFontEx(SCREEN_TV, 0, 10, "Try again...");
                    OSScreenPutFontEx(SCREEN_DRC, 0, 10, "Try again...");
                    OSScreenFlipBuffersEx(SCREEN_TV);
                    OSScreenFlipBuffersEx(SCREEN_DRC);
                    OSSleepTicks(OSMillisecondsToTicks(2000));
                    
                    reset_input();
                    timeout_counter = 0;
                }
            }
            
            if (pressed & VPAD_BUTTON_HOME) {
                reset_input();
                timeout_counter = 0;
            }
        }
        
        // Check timeout - auto-continue to not block boot forever
        timeout_counter++;
        if (timeout_counter >= TIMEOUT_LIMIT) {
            // Show timeout message
            OSScreenClearBufferEx(SCREEN_TV, 0x0000FF);
            OSScreenClearBufferEx(SCREEN_DRC, 0x0000FF);
            OSScreenPutFontEx(SCREEN_TV, 0, 8, "SECURITY TIMEOUT");
            OSScreenPutFontEx(SCREEN_DRC, 0, 8, "SECURITY TIMEOUT");
            OSScreenPutFontEx(SCREEN_TV, 0, 10, "Continuing boot anyway...");
            OSScreenPutFontEx(SCREEN_DRC, 0, 10, "Continuing boot anyway...");
            OSScreenFlipBuffersEx(SCREEN_TV);
            OSScreenFlipBuffersEx(SCREEN_DRC);
            OSSleepTicks(OSMillisecondsToTicks(2000));
            
            password_verified = true; // Exit loop
        }
        
        OSSleepTicks(OSMillisecondsToTicks(50));
    }
    
    // Clear screen and show handoff message
    OSScreenClearBufferEx(SCREEN_TV, 0x000000);
    OSScreenClearBufferEx(SCREEN_DRC, 0x000000);
    OSScreenPutFontEx(SCREEN_TV, 0, 8, "Security check complete");
    OSScreenPutFontEx(SCREEN_DRC, 0, 8, "Security check complete");
    OSScreenPutFontEx(SCREEN_TV, 0, 10, "Loading next module...");
    OSScreenPutFontEx(SCREEN_DRC, 0, 10, "Loading next module...");
    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);
    
    // Small delay to show the message
    OSSleepTicks(OSMillisecondsToTicks(1000));
    
    // Clean up our memory but DON'T disable screens
    // Let the next setup module handle screen management
    if (tvBuffer) MEMFreeToDefaultHeap(tvBuffer);
    if (drcBuffer) MEMFreeToDefaultHeap(drcBuffer);
    
    // Return 0 to continue Tiramisu boot sequence
    // Next module (50_hbl_installer.rpx) will be loaded
    return 0;
}