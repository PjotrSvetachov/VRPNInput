/*
The MIT License (MIT)

Copyright (c) 2015 University of Groningen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include "IMotionController.h"

#if PLATFORM_WINDOWS
	#include "AllowWindowsPlatformTypes.h"
		#include "vrpn_Tracker.h"
		#include "vrpn_Button.h"
		#include "vrpn_Analog.h"
	#include "HideWindowsPlatformTypes.h"    
#elif PLATFORM_LINUX
	#include "vrpn_Tracker.h"
	#include "vrpn_Button.h"
	#include "vrpn_Analog.h"
#endif

class IVRPNInputDevice
{
public:
	IVRPNInputDevice(FCriticalSection& InCritSect) :CritSect(InCritSect){}
	virtual ~IVRPNInputDevice(){};
	virtual void Update() = 0;
	virtual bool ParseConfig(FConfigSection *InConfigSection) = 0;
protected:
	FCriticalSection& CritSect;
};

/*
 * Connects to a VRPN button device.
 * gives an event for each putton up/down press
 */
class VRPNButtonInputDevice : public IVRPNInputDevice
{
public:
	/* If a device is not enabled it will still add the blueprints functions but it does not establish a VRPN connection.
	*/
	VRPNButtonInputDevice(const FString &TrackerAddress, FCriticalSection& InCritSect, bool bEnabled = true);
	virtual ~VRPNButtonInputDevice();

	void Update() override;
	bool ParseConfig(FConfigSection *InConfigSection) override;

private:
	// because key presses callbacks be called in the render thread
	// (due to motion controllers that call mainloop() in reanderthread)
	// we keep a stack to process in the game thread
	struct KeyEventPair{
		vrpn_int32 Button;
		vrpn_int32 State;
	};

	TArray<KeyEventPair> KeyPressStack;

	vrpn_Button_Remote *InputDevice;

	static void VRPN_CALLBACK HandleButtonDevice(void *userData, vrpn_BUTTONCB const b);

	TMap<int32, const FKey> ButtonMap;
};

/*
* Connects to a VRPN tracker device.
* each time update is called it will refresh it's value with the latest.
*/
class VRPNTrackerInputDevice : public IVRPNInputDevice, public IMotionController
{
public:
	/* If a device is not enabled it will still add the blueprints functions but it does not establish a VRPN connection.
	 */
	VRPNTrackerInputDevice(const FString &TrackerAddress, FCriticalSection& InCritSect, bool bEnabled = true);
	virtual ~VRPNTrackerInputDevice();

	void Update() override;
	bool ParseConfig(FConfigSection *InConfigSection) override;

	// IMotionController overrides
	virtual bool GetControllerOrientationAndPosition(const int32 ControllerIndex, const EControllerHand DeviceHand, FRotator& OutOrientation, FVector& OutPosition) const override;

	virtual ETrackingStatus GetControllerTrackingStatus(const int32 ControllerIndex, const EControllerHand DeviceHand) const override;

private:
	struct TrackerInput
	{
		FKey MotionXKey;
		FKey MotionYKey;
		FKey MotionZKey;

		FKey RotationYawKey;
		FKey RotationPitchKey;
		FKey RotationRollKey;

		FVector CurrentTrackerPosition;
		FQuat CurrentTrackerRotation;

		// for motion controllers
		int PlayerIndex;
		EControllerHand Hand;

		bool TrackerDataDirty;
	};

	// Applies the translation and rotations offsets to the tracker coordinates
	void TransformCoordinates(const TrackerInput &Tracker, FVector &OutPosition, FQuat &OutRotation) const;

	vrpn_Tracker_Remote *InputDevice;

	TMap<int32, TrackerInput> TrackerMap;
	FVector TranslationOffset;
	FQuat RotationOffset; // This rotation will be added to the Yaw/Pitch/Roll
	
	float TrackerUnitsToUE4Units;
	bool FlipZAxis;


	static void VRPN_CALLBACK HandleTrackerDevice(void *userData, vrpn_TRACKERCB const tr);
};

/*Connects to a VRPN analog device.
This contains signals, see openVibe for examples.
*/
class VRPNAnalogInputDevice : public IVRPNInputDevice
{
public : 
	VRPNAnalogInputDevice(const FString &TrackerAddress, FCriticalSection& InCritSect, bool bEnabled = true);
	virtual ~VRPNAnalogInputDevice();
	void Update() override;
	bool ParseConfig(FConfigSection *InConfigSection) override;
private: 
	struct ChannelInput
	{
		
	};
	vrpn_Analog_Remote *InputDevice;
	vrpn_int32 num_channel;                 // how many channels
	vrpn_float64 channels[vrpn_CHANNEL_MAX]; // analog values
	TMap<int32, const FKey> ChannelMap;
	static void VRPN_CALLBACK HandleAnalogDevice(void *userData, vrpn_ANALOGCB const tr);
};