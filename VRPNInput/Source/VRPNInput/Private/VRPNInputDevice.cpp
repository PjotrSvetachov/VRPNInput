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

#include "VRPNInputPrivatePCH.h"
#include "VRPNInputDevice.h"

//--------------------------------BUTTON-----------------------------

VRPNButtonInputDevice::VRPNButtonInputDevice(const FString &TrackerAddress, FCriticalSection& InCritSect, bool bEnabled):
IVRPNInputDevice(InCritSect),
InputDevice(nullptr)
{
	KeyPressStack.Reserve(16);
	if(bEnabled){
		InputDevice = new vrpn_Button_Remote(TCHAR_TO_UTF8(*TrackerAddress));
		//InputDevice->shutup = true;
		InputDevice->register_change_handler(this, &VRPNButtonInputDevice::HandleButtonDevice);
	}
}

VRPNButtonInputDevice::~VRPNButtonInputDevice() {
	delete InputDevice;
}

void VRPNButtonInputDevice::Update() {
	if(InputDevice){
		{
			FScopeLock ScopeLock(&CritSect);
			InputDevice->mainloop();
		}
		while(KeyPressStack.Num() > 0)
		{
			KeyEventPair ButtonEvent = KeyPressStack.Pop(/*bAllowShrinking=*/false);
			// process the button presses
			const FKey* Key = ButtonMap.Find(ButtonEvent.Button);
			if(Key == nullptr)
			{
				UE_LOG(LogVRPNInputDevice, Warning, TEXT("Could not find button with id %i."), ButtonEvent.Button);
				return;
			}


			if(ButtonEvent.State == 1)
			{
				FKeyEvent KeyEvent(*Key, FSlateApplication::Get().GetModifierKeys(), 0, 0, 0, 0);
				FSlateApplication::Get().ProcessKeyDownEvent(KeyEvent);
			}
			else
			{
				FKeyEvent KeyEvent(*Key, FSlateApplication::Get().GetModifierKeys(), 0, 0, 0, 0);
				FSlateApplication::Get().ProcessKeyUpEvent(KeyEvent);
			}
		}
	}
}

bool VRPNButtonInputDevice::ParseConfig(FConfigSection *InConfigSection) {
	TArray<const FConfigValue*> Buttons;
	InConfigSection->MultiFindPointer(FName(TEXT("Button")), Buttons);
	if(Buttons.Num() == 0)
	{
		UE_LOG(LogVRPNInputDevice, Warning, TEXT("Config file for button device has no button mappings specified. Expeted field Button."));
		return false;
	}
	
	for(const FConfigValue* ButtonString: Buttons)
	{
		int32 ButtonId;
		FString ButtonName;
		FString ButtonDescription;
		if(!FParse::Value(*ButtonString->GetValue(), TEXT("Id="), ButtonId) ||
		   !FParse::Value(*ButtonString->GetValue(), TEXT("Name="), ButtonName) ||
		   !FParse::Value(*ButtonString->GetValue(), TEXT("Description="), ButtonDescription))
		{
			UE_LOG(LogVRPNInputDevice, Warning, TEXT("Config not parse button. Expected: Button = (Id=#,Name=String,Description=String)."));
			continue;
		}
		// While NewKey is only valid for this iteration it will be copied in FKeyDetails below
		const FKey &NewKey = ButtonMap.Add(ButtonId, FKey(*ButtonName));
		EKeys::AddKey(FKeyDetails(NewKey, FText::FromString(ButtonDescription), FKeyDetails::GamepadKey));
	}

	return ButtonMap.Num() > 0;
}

void VRPN_CALLBACK VRPNButtonInputDevice::HandleButtonDevice(void *userData, vrpn_BUTTONCB const b) {
	VRPNButtonInputDevice &ButtonDevice = *reinterpret_cast<VRPNButtonInputDevice*>(userData);
	ButtonDevice.KeyPressStack.Push({b.button,b.state});
}

//--------------------------------TRACKER-----------------------------

VRPNTrackerInputDevice::VRPNTrackerInputDevice(const FString &TrackerAddress, FCriticalSection& InCritSect, bool bEnabled):
IVRPNInputDevice(InCritSect),
InputDevice(nullptr),
TranslationOffset(0,0,0),
RotationOffset(EForceInit::ForceInit),
TrackerUnitsToUE4Units(1.0f),
FlipZAxis(false)
{
	if(bEnabled){
		InputDevice = new vrpn_Tracker_Remote(TCHAR_TO_UTF8(*TrackerAddress));
		//InputDevice->shutup = true;
		InputDevice->register_change_handler(this, &VRPNTrackerInputDevice::HandleTrackerDevice);
	}
}

VRPNTrackerInputDevice::~VRPNTrackerInputDevice() {
	delete InputDevice;
}

void VRPNTrackerInputDevice::Update() {
	if(InputDevice){
		{
			FScopeLock ScopeLock(&CritSect);
			InputDevice->mainloop();
		}
		for(auto &InputPair : TrackerMap)
		{
			TrackerInput &Input = InputPair.Value;
			if(Input.TrackerDataDirty)
			{
				// Before firing events, transform the tracker into the right coordinate space

				FVector NewPosition;
				FQuat NewRotation;
				TransformCoordinates(Input, NewPosition, NewRotation);

				FRotator NewRotator = NewRotation.Rotator();

				FAnalogInputEvent AnalogInputEventX(Input.MotionXKey, FSlateApplication::Get().GetModifierKeys(), 0, 0, 0, 0, NewPosition.X);
				FSlateApplication::Get().ProcessAnalogInputEvent(AnalogInputEventX);
				FAnalogInputEvent AnalogInputEventY(Input.MotionYKey, FSlateApplication::Get().GetModifierKeys(), 0, 0, 0, 0, NewPosition.Y);
				FSlateApplication::Get().ProcessAnalogInputEvent(AnalogInputEventY);
				FAnalogInputEvent AnalogInputEventZ(Input.MotionZKey, FSlateApplication::Get().GetModifierKeys(), 0, 0, 0, 0, NewPosition.Z);
				FSlateApplication::Get().ProcessAnalogInputEvent(AnalogInputEventZ);

				FAnalogInputEvent AnalogInputEventRotX(Input.RotationYawKey, FSlateApplication::Get().GetModifierKeys(), 0, 0, 0, 0, NewRotator.Yaw);
				FSlateApplication::Get().ProcessAnalogInputEvent(AnalogInputEventRotX);
				FAnalogInputEvent AnalogInputEventRotY(Input.RotationPitchKey, FSlateApplication::Get().GetModifierKeys(), 0, 0, 0, 0, NewRotator.Pitch);
				FSlateApplication::Get().ProcessAnalogInputEvent(AnalogInputEventRotY);
				FAnalogInputEvent AnalogInputEventRotZ(Input.RotationRollKey, FSlateApplication::Get().GetModifierKeys(), 0, 0, 0, 0, NewRotator.Roll);
				FSlateApplication::Get().ProcessAnalogInputEvent(AnalogInputEventRotZ);

				Input.TrackerDataDirty = false;
			}
		}
	}
}

bool VRPNTrackerInputDevice::ParseConfig(FConfigSection *InConfigSection) {
	FConfigValue *RotationOffsetConfigValue = InConfigSection->Find(FName(TEXT("RotationOffset")));
	RotationOffset = FQuat::Identity;
	if(RotationOffsetConfigValue)
	{
		const FString &RotationOffsetString = RotationOffsetConfigValue->GetValue();

		FVector RotationOffsetVector;
		float RotationOffsetAngleDegrees;
		if(!RotationOffsetVector.InitFromString(RotationOffsetString) || !FParse::Value(*RotationOffsetString, TEXT("Angle="), RotationOffsetAngleDegrees))
		{
			UE_LOG(LogVRPNInputDevice, Log, TEXT("Expected RotationOffsetAxis of type FVector and RotationOffsetAnlge of type Float when parsing tracker device. Rotation offset will be the indenty transform."));
		}
		else
		{
			RotationOffsetVector.Normalize();
			RotationOffset = FQuat(RotationOffsetVector, FMath::DegreesToRadians(RotationOffsetAngleDegrees));
			RotationOffset.Normalize();
		}
	}
	else
	{
		UE_LOG(LogVRPNInputDevice, Log, TEXT("Expected RotationOffsetAxis of type FVector and RotationOffsetAnlge of type Float when parsing tracker device. Rotation offset will be the indenty transform."));
	}

	FConfigValue *PositioOffsetConfigValue = InConfigSection->Find(FName(TEXT("PositionOffset")));
	if(PositioOffsetConfigValue == nullptr || !TranslationOffset.InitFromString(PositioOffsetConfigValue->GetValue()))
	{
		UE_LOG(LogVRPNInputDevice, Log, TEXT("Expected PositionOffset of type FVector (in device coordinates). Will use default of (0,0,0)."));
		TranslationOffset.Set(0, 0, 0);
	}

	FConfigValue *TrackerUnitsToUE4UnitsConfigValue = InConfigSection->Find(FName(TEXT("TrackerUnitsToUE4Units")));
	if(TrackerUnitsToUE4UnitsConfigValue == nullptr)
	{
		UE_LOG(LogVRPNInputDevice, Warning, TEXT("Expected to find TrackerUnitsToUE4UnitsText of type Float. Using default of 1.0f"));
		TrackerUnitsToUE4Units = 1.0f;
	} else
	{
		TrackerUnitsToUE4Units = FCString::Atof(*TrackerUnitsToUE4UnitsConfigValue->GetValue());
	}

	FConfigValue *FlipZAxisConfigValue = InConfigSection->Find(FName(TEXT("FlipZAxis")));
	if(FlipZAxisConfigValue == nullptr)
	{
		UE_LOG(LogVRPNInputDevice, Warning, TEXT("Expected to find FlipZAxis of type Boolean. Using default of false."));
		FlipZAxis = false;
	} else
	{
		FlipZAxis = FCString::ToBool(*InConfigSection->Find(FName(TEXT("FlipZAxis")))->GetValue());
	}

	TArray<const FConfigValue*> Trackers;
	InConfigSection->MultiFindPointer(FName(TEXT("Tracker")), Trackers);
	if(Trackers.Num() == 0)
	{
		UE_LOG(LogVRPNInputDevice, Warning, TEXT("Config file for tracker device has no tracker mappings specified. Expeted field Tracker."));
		return false;
	}

	bool bHasMotionControllers = false;

	for(const FConfigValue* TrackerString: Trackers)
	{
		int32 TrackerId;
		FString TrackerName;
		FString TrackerDescription;
		if(!FParse::Value(*TrackerString->GetValue(), TEXT("Id="), TrackerId) ||
		   !FParse::Value(*TrackerString->GetValue(), TEXT("Name="), TrackerName) ||
		   !FParse::Value(*TrackerString->GetValue(), TEXT("Description="), TrackerDescription))
		{
			UE_LOG(LogVRPNInputDevice, Warning, TEXT("Config not parse tracker. Expected: Tracker = (Id=#,Name=String,Description=String)."));
			continue;
		}
		
		// see if this is a motion controller
		int PlayerId = -1;
		FParse::Value(*TrackerString->GetValue(), TEXT("PlayerId="), PlayerId);
		EControllerHand Hand = EControllerHand::Left;
		if(PlayerId >= 0)
		{
			UE_LOG(LogVRPNInputDevice, Log, TEXT("Found motion controller."));
			bHasMotionControllers = true;
			FString HandString;
			if(FParse::Value(*TrackerString->GetValue(), TEXT("Hand="), HandString))
			{
				if(HandString.Equals("Right"))
				{
					Hand = EControllerHand::Right;
				}
			}
		}

		
		UE_LOG(LogVRPNInputDevice, Log, TEXT("Adding new tracker: [%i,%s,%s,%i]."), TrackerId, *TrackerName, *TrackerDescription, PlayerId);
		
		const TrackerInput &Input = TrackerMap.Add(TrackerId, {FKey(*(TrackerName + "MotionX")), FKey(*(TrackerName + "MotionY")), FKey(*(TrackerName + "MotionZ")),
													FKey(*(TrackerName + "RotationYaw")), FKey(*(TrackerName + "RotationPitch")), FKey(*(TrackerName + "RotationRoll")),
													FVector(0), FQuat(EForceInit::ForceInit), PlayerId, Hand,false});

		// Translation
		EKeys::AddKey(FKeyDetails(Input.MotionXKey, FText::FromString(TrackerName + " X position"), FKeyDetails::FloatAxis));
		EKeys::AddKey(FKeyDetails(Input.MotionYKey, FText::FromString(TrackerName + " Y position"), FKeyDetails::FloatAxis));
		EKeys::AddKey(FKeyDetails(Input.MotionZKey, FText::FromString(TrackerName + " Z position"), FKeyDetails::FloatAxis));

		// Rotation
		EKeys::AddKey(FKeyDetails(Input.RotationYawKey, FText::FromString(TrackerName + " Yaw"), FKeyDetails::FloatAxis));
		EKeys::AddKey(FKeyDetails(Input.RotationPitchKey, FText::FromString(TrackerName + " Pitch"), FKeyDetails::FloatAxis));
		EKeys::AddKey(FKeyDetails(Input.RotationRollKey, FText::FromString(TrackerName + " Roll"), FKeyDetails::FloatAxis));
	}

	if(bHasMotionControllers)
	{
		UE_LOG(LogVRPNInputDevice, Log, TEXT("Adding this to the motion controller devices."));
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	}


	return true;
}

void VRPNTrackerInputDevice::TransformCoordinates(const TrackerInput &Tracker, FVector &OutPosition, FQuat &OutRotation) const
{
	FVector NewPosition = Tracker.CurrentTrackerPosition;
	FVector NewTranslationOffset = TranslationOffset;
	if(FlipZAxis)
	{
		NewPosition.Z = -NewPosition.Z;
		NewTranslationOffset.Z = -NewTranslationOffset.Z;
	}

	static UWorld* OurWorld = nullptr;
	if(OurWorld == nullptr) {
		for(const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if(Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::Editor)
			{
				OurWorld = Context.World();
				break;
			}
		}
	}

	float WorldToScale = 1.0f;
	if(OurWorld && OurWorld->GetWorldSettings()) {
		WorldToScale = OurWorld->GetWorldSettings()->WorldToMeters * 0.01f;
	}
	OutPosition = RotationOffset.RotateVector((NewPosition + NewTranslationOffset)*TrackerUnitsToUE4Units*WorldToScale);
	FQuat NewRotation = Tracker.CurrentTrackerRotation;
	if(FlipZAxis)
	{
		NewRotation.X = -NewRotation.X;
		NewRotation.Y = -NewRotation.Y;
	}
	OutRotation = RotationOffset*NewRotation;
}

bool VRPNTrackerInputDevice::GetControllerOrientationAndPosition(const int32 ControllerIndex, const EControllerHand DeviceHand, FRotator& OutOrientation, FVector& OutPosition) const
{
	for(auto &InputPair : TrackerMap)
	{
		const TrackerInput &Tracker = InputPair.Value;
		if(Tracker.PlayerIndex == ControllerIndex && Tracker.Hand == DeviceHand)
		{
			if(InputDevice)
			{
				FScopeLock ScopeLock(&CritSect);
				InputDevice->mainloop();
			}

			FVector NewPosition;
			FQuat NewRotation;
			TransformCoordinates(Tracker, NewPosition, NewRotation);

			OutOrientation = NewRotation.Rotator();
			OutPosition = NewPosition;
			return true;
		}
	}

	return false;
}

// for now always return tracked, need to see later if we can return better information
ETrackingStatus VRPNTrackerInputDevice::GetControllerTrackingStatus(const int32 ControllerIndex, const EControllerHand DeviceHand) const {
	return ETrackingStatus::Tracked;
}

void VRPN_CALLBACK VRPNTrackerInputDevice::HandleTrackerDevice(void *userData, vrpn_TRACKERCB const tr) {
	VRPNTrackerInputDevice &TrackerDevice = *reinterpret_cast<VRPNTrackerInputDevice*>(userData);
	TrackerInput *Input = TrackerDevice.TrackerMap.Find(tr.sensor);
	if(Input == nullptr)
	{
		UE_LOG(LogVRPNInputDevice, Warning, TEXT("Could not find tracker with id %i."), tr.sensor);
		return;
	}

	Input->TrackerDataDirty = true;

	Input->CurrentTrackerPosition.X = tr.pos[0];
	Input->CurrentTrackerPosition.Y = tr.pos[1];
	Input->CurrentTrackerPosition.Z = tr.pos[2];

	FQuat NewRotation = FQuat(tr.quat[0], tr.quat[1], tr.quat[2], tr.quat[3]);

	Input->CurrentTrackerRotation = FQuat::Slerp(Input->CurrentTrackerRotation, NewRotation, 0.2f);

	Input->CurrentTrackerRotation.X = tr.quat[0];
	Input->CurrentTrackerRotation.Y = tr.quat[1];
	Input->CurrentTrackerRotation.Z = tr.quat[2];
	Input->CurrentTrackerRotation.W = tr.quat[3];
}

//--------------------------------ANALOG-----------------------------

VRPNAnalogInputDevice::VRPNAnalogInputDevice(const FString & TrackerAddress, FCriticalSection & InCritSect, bool bEnabled):
IVRPNInputDevice(InCritSect),
InputDevice(nullptr)
{
	if (bEnabled) {
		InputDevice = new vrpn_Analog_Remote(TCHAR_TO_UTF8(*TrackerAddress));
		//InputDevice->shutup = true;
		InputDevice->register_change_handler(this, &VRPNAnalogInputDevice::HandleAnalogDevice);
	}
}

VRPNAnalogInputDevice::~VRPNAnalogInputDevice(){
	delete InputDevice;
}

void VRPNAnalogInputDevice::Update()
{
	if (InputDevice) {
		{
			FScopeLock ScopeLock(&CritSect);
			InputDevice->mainloop();
		}
		for (int a = 0; a < num_channel; a = a + 1)
		{
			const FKey* Key = ChannelMap.Find(a);
			if (Key == nullptr)
			{
				UE_LOG(LogVRPNInputDevice, Warning, TEXT("Could not find button with id %i."), a);
				return;
			}
			FAnalogInputEvent AnalogChannel(*Key, FSlateApplication::Get().GetModifierKeys(), 0, 0, 0, 0, channels[a]);
			FSlateApplication::Get().ProcessAnalogInputEvent(AnalogChannel);
		}
	}
}

bool VRPNAnalogInputDevice::ParseConfig(FConfigSection * InConfigSection)
{
	TArray<const FConfigValue*> Channels;
	InConfigSection->MultiFindPointer(FName(TEXT("Channel")), Channels);
	if (Channels.Num() == 0)
	{
		UE_LOG(LogVRPNInputDevice, Warning, TEXT("Config file for analog device has no channel mappings specified. Expeted field Channel."));
		return false;
	}

	for (const FConfigValue* ChannelString : Channels)
	{
		int32 ChannelId;
		FString ChannelName;
		FString ChannelDescription;
		if (!FParse::Value(*ChannelString->GetValue(), TEXT("Id="), ChannelId) ||
			!FParse::Value(*ChannelString->GetValue(), TEXT("Name="), ChannelName) ||
			!FParse::Value(*ChannelString->GetValue(), TEXT("Description="), ChannelDescription))
		{
			UE_LOG(LogVRPNInputDevice, Warning, TEXT("Config not parse channel. Expected: Channel = (Id=#,Name=String,Description=String)."));
			continue;
		}
		// While NewKey is only valid for this iteration it will be copied in FKeyDetails below
		const FKey &NewKey = ChannelMap.Add(ChannelId, FKey(*ChannelName));
		EKeys::AddKey(FKeyDetails(NewKey, FText::FromString(ChannelDescription), FKeyDetails::FloatAxis));
	}

	return ChannelMap.Num() > 0;
}

void VRPN_CALLBACK VRPNAnalogInputDevice::HandleAnalogDevice(void * userData, vrpn_ANALOGCB const an)
{
	VRPNAnalogInputDevice &AnalogDevice = *reinterpret_cast<VRPNAnalogInputDevice*>(userData);
	AnalogDevice.num_channel = an.num_channel;
	for (int a = 0; a < AnalogDevice.num_channel; a = a + 1)
	{
		AnalogDevice.channels[a] = an.channel[a];
	}
}
