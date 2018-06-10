# MouseScrollWheelReverser
Simple Qt Windows program that allows the FlipFlopWheel registry entry to be set to reverse the mouse scroll wheel

Basically this program does the same as the the powershell line:

Get-ItemProperty HKLM:\SYSTEM\CurrentControlSet\Enum\HID\*\*\Device` Parameters FlipFlopWheel -EA 0 | ForEach-Object { Set-ItemProperty $_.PSPath FlipFlopWheel 1 }

But has to do it the hard way using RegGetValueW and RegSetKeyValueW. 

In fact it is worse than that because there are no wild cards in the standard API so it has to use RegOpenKeyExW and RegEnumKeyExW to iterate through the various entries where powershell can just use * and of course it has to use std::wstring all the way through to cope with Unicode.

It also plays with the manifest to force the program to run as administrator.

This ia a moderately dangerous piece of code. It was fun to write and illustrates how to do edit the Windows registry and get a Qt program to run as administrator. It seems to work OK but one mistake and a program like this could completely trash your registry! I've uploaded a binary but it would be much safer to compile up your own copy after you've checked my code to see that it does what you expect.
