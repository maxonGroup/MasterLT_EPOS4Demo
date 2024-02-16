#ifdef __cplusplus
extern "C"
{
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S3 // MicroMaster LT uses ESP32S3, MiniMaster LT uses ESP32
    // no user LEDs on Micromaster
#else
// three user LEDs on MiniMaster LT. These pins can also be used as general purpose.
#define LED_GPIO_RED GPIO_NUM_13
#define LED_GPIO_GREEN GPIO_NUM_4
#define LED_GPIO_BLUE GPIO_NUM_16
#endif

    // Top level function. Launches all starting code.
    void app_main();

#ifdef __cplusplus
}
#endif
