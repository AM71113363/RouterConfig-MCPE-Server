#if you dont know how portforwarding works, this "readme" its not for YOU.


build.bat will work only with Dev-C++ 4.9.9.*

//MCPE version 1.16.*
1.Start MCPE with Multiplayer [ON] (LogOut,you can use it without Xbox Login[ You can change your name ] )
2.run RouterConfig.exe (32Bit Windows) (You can Edit port number)(default = 19132)
3.Click [ Ping ].[ Ping have Timeout of 1 second,if it doesn't detect local servers click it again ]
4.Click on [Server1] , [Server2], [Server3], [Server4] to get the Details for the Router Settings.
5.Open your Router and add PortForwarding RULE.
6.Done

NOTE:
Each time you run MCPE as Server the port will change,you have to run RouterConfig.exe to get the current Port
and change the Router Settings.

IMPORTANT:
If Ping fails enable option [ bind ].(FIREWALL WARNING)

#SETTINGS (example "12345" is the port in IntPort field)
         ExtPort              IntPort
    Start      End       Start       End
    19132      19132     12345       12345

Add two port forwarding rules
        ExtPort               IntPort 
    Start      End       Start       End 
    19132      19132     19132       19132
    12345      12345     12345       12345

 #Share hostname:19132, and it always work.
Just change the port 12345 whenever the port changes.

