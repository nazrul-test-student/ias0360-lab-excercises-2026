// Force-override the weak hook used inside busy_wait_ms()
// The SDK defines a weak symbol __real_tight_loop_contents()
// and calls tight_loop_contents(), which we can override.

void tight_loop_contents(void)
{
    // Do nothing — prevents the default WFI sleep
}