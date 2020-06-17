==========================
 Half-LifeX Pre-Beta 0.85
==========================

Gbrownie
-Fixed controls
	-Fixed overly sensitive thumbsticks (would instantly go to max speed at slightest nudge)
	-Fixed weird camera jerking if thumbsticks were held at certain angles
	-Allows multiple buttons to be pressed at a time (previously only one event would be processed at a time, effectively
							  blocking other buttons)
	-Enabled clicking in controller sticks and mapping those sticks to controls (use 'XBOX_RSTICK/XBOX_LSTICK' in config.cfg)
	-Fixed ducking, ducking was very glitchy as the toggle was done incorrectly
-Fixed menu highlight to mimic vanilla half life more closely
	-Highlights stay active as if you held the mouse on it (previously the highlight would fade out after a few seconds

-Other small changes
----

This is a port of Half-Life 1 to the original Xbox using Xash3D and FakeGLx.

The following included libraries need to be compiled first before building Half-LifeX.

Client: Using XashXT client as the Valve version has a closed source library for VGUI.
Server: Using the Valve SDK server library v2.3.
MenuUI: The main menu for Xash3D to replicate the original WON menu.

Currently only runs on 128MB modded Xboxes, I will be reducing the memory footprint and attempting to use virtual memory in future releases to allow it to run on stock 64mb Xboxes. 

The port is not finish yet but only minor things need to be completed, this is very playable in it's current state using 128MB modded Xboxes.
