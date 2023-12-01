# usbModel

The usbModel is a software model of both a USB host and device to standard USB 1.1, with hooks for USB 2.0 enhancements. Each model can be used independently, with the host model a generic implementation, and the device model an example of a specific communications device class (CDC) implementation. The models have been integrated with the [VProc Virtual Processor](https://github.com/wyvernSemi/vproc) to drive USB signals in a Verilog simulation, with scripts for running on ModelSim. This allows the model to be used to drive device, hub, and host logic implementations.

<img src="https://github.com/wyvernSemi/usbModel/assets/21970031/41ae5119-1ca6-44ed-bc0c-f7b76ad2d5cc" width=800>

## Host Model Features

*	Automatic checks for connection/disconnection.
*	Automatic generation of SOF token packets each frame.
*	Ability to generate control transactions.
    *	Get device, interface, endpoint, string, and class specific descriptors.
    *	Set device address.
    *	Get device, interface, and endpoint statuses.
    *	Get and set device configuration (enable/disable).
    *	Get and set interface alternative.
    *	Set and clear device, interface, and endpoint features.
    *	Get endpoint synch frame state.
*	Generate bulk and isochronous OUT packets.
*	Generate bulk and isochronous IN packets.
*	Suspend a device.
*	Reset a device.
*	Display formatted output of received packet data.

## Device Model Features

*	Ability to connect and disconnect from USB line.
*	Suspension detection.
*	Reset detection.
*	Implements a CDC device with:
    *	One device configuration.
    *	Two interfaces.
    *	Three endpoints.
        *	One a notify endpoint on one interface.
        *	Two data endpoints (IN and OUT) on the other interface.
*	Responses to control packets.
*	Bulk data transfers IN and OUT.
    *	A user callback can be registered at construction, called for each transfer request received.
*	Display formatted output of received packet data.

<br>
<img src="https://github.com/wyvernSemi/usbModel/assets/21970031/1cbb0edb-b50f-42c7-af89-3afb6791f045" width=453>
