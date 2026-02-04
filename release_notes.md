# `AUTO-A113` Engine Release Notes

## 4 Feb 2026 - `v1.0.3`
> - Added support for bluetooth adresses in IO utils.
> - Deprecated IO utils address string types' get(), use str() instead. ( avoid confusion in inheritance cases ).

## 25 Jan 2026 - `v1.0.2`
> - Modified the `cache::Bucket` structure and some logic behind it, including the `BucketHandle_` control flow.
> - Completed the texture cache for the immersive render cluster.

## 16 Jan 2026 - `v1.0.1`
> - Added a `drop` method to the `Dispenser` which is supposed to cancel the changes made under a control acquire. This method currently supports only `DispenserMode_Drop`. No, the method name has nothing to do with the mode name, it is just a coincidence :).

> - Added the posibility to register callbacks which are invoked when `io::COM_Ports` is refreshed.

> - Fixed a bug in `imm_widgets::COM_Ports` where when unplugging a device from the selected COM port would in some cases create inconsistencies in the selected port. 

> - Added extended error logging methods.

## 31 Dec 2025 - `v1.0.0`
> - Baremetal release of the engine.