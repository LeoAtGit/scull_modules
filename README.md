# scull_modules
Just some modules for the purpose of learning about the world of device drivers in Linux. 

Contents:

	- scull0/ is a simple driver that sits in /dev/scull0 and to which you can write to as well as read the stuff you wrote to it again
	- scull1/ is a simple driver in which you can change around the different branches to see different implementations of the locking of resources
		  You can switch between branches by using the command
		  	
			git checkout origin/name_of_branch

		  and you can see the name of the branch when you first pull from it or in the web interface. And of course it is listed here:

 			- completion
 			- deadlock
 			- deadlock_not_interruptible


