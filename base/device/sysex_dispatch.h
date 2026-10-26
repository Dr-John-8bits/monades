/* SPDX-License-Identifier: MPL-2.0
 * Mutable Dreams: one owner for each pending USB SysEx frame.
 * An IRQ may receive the next frame immediately after ota_frame_done().
 * Do not call ota_service() again after an application handler consumed one:
 * it would discard that newly arrived application frame as invalid OTA data.
 */
static void fm1_dispatch_sysex(uint32_t now)
{
    const uint8_t *p;
    uint32_t n;
    if (!ota_frame_get(&p, &n))
        return;
    if (ann_frame(p, n, now) || log_frame(p, n) || remote_frame(p, n)
#if FM1B_WIFI
        || wifi_frame(p, n)
#endif
#ifdef APP_SYSEX
        || app_sysex(p, n)
#endif
    )
        ota_frame_done();
    else
        usb_cmd(ota_service()); /* The same still-pending frame; never its successor. */
}
