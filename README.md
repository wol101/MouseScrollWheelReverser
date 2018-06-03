# MouseScrollWheelReverser
Simple Qt Windows program that allows the FlipFlopWheel registry entry to be set to reverse the mouse scroll wheel

Basically this program does the same as the the powershell line

Get-ItemProperty HKLM:\SYSTEM\CurrentControlSet\Enum\HID\*\*\Device` Parameters FlipFlopWheel -EA 0 | ForEach-Object { Set-ItemProperty $_.PSPath FlipFlopWheel 1 }

But has to do it the hard way using RegGetValueW and RegSetKeyValueW. 

In fact it is worse than that because there are no wild cards in the standard API so it has to use RegOpenKeyExW and RegEnumKeyExW to iterate through the various entries where powershell can just use * and of course it has to use std::wstring all the way through to cope with Unicode.

It also plays with the manifest so that it has to run as administrator. A moderately dangerous piece of code.
