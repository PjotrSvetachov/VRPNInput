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
#include "VRPNInputDeviceManager.h"
#if PLATFORM_WINDOWS
	#include "AllowWindowsPlatformTypes.h"
		#include "vrpn_Tracker.h"
		#include "vrpn_Button.h"
	#include "HideWindowsPlatformTypes.h"    
#elif PLATFORM_LINUX
	#include "vrpn_Tracker.h"
	#include "vrpn_Button.h"
#endif
DEFINE_LOG_CATEGORY(LogVRPNInputDevice);

class FVRPNInputPlugin : public IInputDeviceModule
{
public:
	/*
	* We override this so we can init the keys before the blueprints are compiled on editor startup
	* if we do not do this here we will get warnings in blueprints.
	*/
	virtual void StartupModule() override {
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);

		UE_LOG(LogVRPNInputDevice, Log, TEXT("Locating VRPN config file."));
		FString ConfigFile;
		if(!FParse::Value(FCommandLine::Get(), TEXT("VRPNConfigFile="), ConfigFile))
		{
			UE_LOG(LogVRPNInputDevice, Warning, TEXT("Could not find VRPN configuration file: %s."), *ConfigFile);
			return;
		}
		if(!FPaths::FileExists(ConfigFile))
		{
			return;
			UE_LOG(LogVRPNInputDevice, Warning, TEXT("Could not find VRPN configuration file: %s."), *ConfigFile);
		}

		TArray<FString> SectionNames;
		GConfig->GetSectionNames(ConfigFile,SectionNames);
		for(FString &SectionNameString : SectionNames)
		{
			// Tracker name is the section name itself
			FConfigSection* TrackerConfig = GConfig->GetSectionPrivate(*SectionNameString, false, true, ConfigFile);

			FString *TrackerTypeString = TrackerConfig->Find(FName(TEXT("Type")));
			if(TrackerTypeString == nullptr)
			{
				UE_LOG(LogVRPNInputDevice, Warning, TEXT("Tracker config file %s: expected to find Type of type String in section [%s]. Skipping this section."), *ConfigFile, *SectionNameString);
				continue;
			}

			FString *TrackerAdressString = TrackerConfig->Find(FName(TEXT("Address")));
			if(TrackerAdressString == nullptr)
			{
				UE_LOG(LogVRPNInputDevice, Warning, TEXT("Tracker config file %s: expected to find Address of type String in section [%s]. Skipping this section."), *ConfigFile, *SectionNameString);
				continue;
			}

			IVRPNInputDevice *InputDevice = nullptr;
			if(TrackerTypeString->Compare("Tracker") == 0)
			{
				InputDevice = new VRPNTrackerInputDevice(SectionNameString, *TrackerAdressString);
			} else if(TrackerTypeString->Compare("Button") == 0)
			{
				InputDevice = new VRPNButtonInputDevice(SectionNameString, *TrackerAdressString);
			} else
			{
				UE_LOG(LogVRPNInputDevice, Warning, TEXT("Tracker config file %s: Type should be Tracker or Button but found %s in section %s. Skipping this section."), *ConfigFile, *(*TrackerTypeString), *SectionNameString);
				continue;
			}
			if(!InputDevice->ParseConfig(TrackerConfig))
			{
				UE_LOG(LogVRPNInputDevice, Warning, TEXT("Tracker config file %s: Could not parse config %s.."), *ConfigFile, *SectionNameString);
				continue;
			}
			if(!DeviceManager.IsValid())
			{
				UE_LOG(LogVRPNInputDevice, Log, TEXT("Create VRPN Input Manager."));
				DeviceManager = TSharedPtr< FVRPNInputDeviceManager >(new FVRPNInputDeviceManager());
			}
			DeviceManager->AddInputDevice(InputDevice);
		}

		
	}

	/** IPsudoControllerInterface implementation */
	virtual TSharedPtr< class IInputDevice > CreateInputDevice(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override {
		if(!DeviceManager.IsValid())
		{
			UE_LOG(LogVRPNInputDevice, Log, TEXT("Create VRPN Input Manager"));
			DeviceManager = TSharedPtr< FVRPNInputDeviceManager >(new FVRPNInputDeviceManager());
		}

		return DeviceManager;
	}

	virtual void ShutdownModule() override {
		DeviceManager = nullptr;
	}


	TSharedPtr< class FVRPNInputDeviceManager > DeviceManager;
};

IMPLEMENT_MODULE(FVRPNInputPlugin, VRPNInput)

FVRPNInputDeviceManager::FVRPNInputDeviceManager() {
}

FVRPNInputDeviceManager::~FVRPNInputDeviceManager() {
	for(IVRPNInputDevice* InputDevice: VRPNInputDevices)
	{
		delete InputDevice;
	}
}

void FVRPNInputDeviceManager::SendControllerEvents() {
	for(IVRPNInputDevice* InputDevice: VRPNInputDevices)
	{
		InputDevice->Update();
	}
}
