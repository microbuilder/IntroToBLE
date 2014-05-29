# Apple Notification Center Service Demo #
[Apple Notification Center Service](https://developer.apple.com/library/ios/documentation/CoreBluetooth/Reference/AppleNotificationCenterServiceSpecification/AppleNotificationCenterServiceSpecification.pdf) is a GATT service that will send push notifications to an connected bluetooth device whenever a notification event occurs on the iOS device.
This code lays out the basic stuff you need to get ANCS working on a project with an embedded device, such as a DIY smartwatch. 

## Setting the Correct Advertising Packet ##
In order to have your device show up in the list of available bluetooth devices in the system settings pane, you need to register your interest. The Punch Through Design [blog post](http://blog.punchthrough.com/post/63658238857/the-apple-notification-center-service-or-wtf-is) on ANCS left a hint about how to do this.  You need to use "Service Solicitation" to indicate that you are interested in subscribing to the ANCS service.

Core 4.0 Vol 3 Part C, 18.9 gives the AD identifier, `$15`.   I included a local name in my packet so that I could identify my device.  In actual byte order, here is the contents of my packet:

`02` AD Field, 2 bytes  
`01` AD Flags  
`02` General Discovery, no BR/EDR, you can change this as needed  
`04` AD Field, 4 bytes  
`09` AD Complete Local Name  
`4C 45 44` LED, not sure why I called it that  
`11` AD Field, 17 bytes  
`15` AD Service Solicitation, 128 bits  
`D0 00 2D 12 1E 4B 0F A4 99 4E CE B5 31 F4 05 79` The ANCS service in LSB to MSB order.

## Pairing ##
Subscribing to events requires that the devices be paired.  Bluegiga has a [doc](https://bluegiga.zendesk.com/entries/22882472--REFERENCE-Bonding-encryption-and-MITM-protection-with-the-BLE112) that I worked from to start encryption when the connection event is fired on the device.  You do not need to have a pin.

## Enumerating the iOS Services  ##
Before you can enumerate the ANCS service, you must find its handle range.  iOS will only return characteristics for the correct service range, so enumerating `$0000` to `$ffff` will not work.

ANCS is a primary service, so listing all primary services will allow you to find it, as well as give you the start and end handles for attribute enumeration.  An excellent bluegiga example [health thermometer collector](https://bluegiga.zendesk.com/entries/23999407--BGScript-htm-collector-Health-Thermometer-collector-BLE-master-), gave me the structure I needed for enumerating the services, enumerating the attributes of the service of interest, subscribing to a notifiable attribute using the Client Characteristic Configuration Descriptor, and receiving new values via an event callback.

## Enumerating the ANCS Service ##
Here are the attributes of the ANCS service, as returned to my device, in the exact order that the bytes given to the buffer, essentially reverse-documentation order.

`00 28` Primary Service

`03 28` Characteristic Declaration  
`d9 d9 aa fd bd 9b 21 98 a8 49 e1 45 f3 d8 d1 69`  ANCS Control Point  
`00 29` Characteristic Extended Properties  

`03 28` Characteristic Declaration  
`bd 1d a2 99 e6 25 58 8c d9 42 01 63 0d 12 bf 9f` ANCS Notification Source  
`02 29` Client Characteristic Configuration Descriptor  

`03 28` Characteristic Declaration  
`fb 7b 7c ce 6a b3 44 be b5 4b d6 24 e9 c6 ea 22` ANCS Data Source  
`02 29` Client Characteristic Configuration Descriptor  

Core 4.0 Vol 3 Part G, 3.4 lists built-in types, and 4.0 Vol 3 Part G, 3.3.3 provides more information on each type.

Save the attribute handles for:
- The ANCS Control Point
- The Client Characteristic Configuration Descriptor for the Notification Source
- The Client Characteristic Configuration Descriptor for the Data Source

## Subscribe to events ##
At minimum, set the Notification Source CCC for notification (indication is not supported) by writing `01 00` to the Notification Source CCC attribute.  If you also want to get textual or other data, first subscribe to the Data Source by setting the Data Source CCC for notification, then set the Notification source after that.  When you receive a notification, get the textual data from the notification by writing a message to the ANCS Control Point attribute.  You will receive a response from the Data Source attribute with the notification id and data requested.

Example incoming notification:

New incoming call (value source is NS handle):

`00` Event ID - Event Added  
`02` Event Flags - Important  
`01` Category ID - Incoming call  
`01` Category Count - 1  
`15 00 00 00` Notification UID

Get the caller id (write this to the CP handle)

`00` Command ID - GetNotificationAttributes  
`15 00 00 00` Notification UID  
`01` Attribute ID - Title  
`0B 00` Attribute max length (11 bytes)  

Response (value source is DS handle)

`00` Command ID - GetNotificationAttributes  
`15 00 00 00` Notification UID  
`01` Attribute ID - Title 
`0A 00` Attribute length (10 bytes)  
`73 6c 61 63 6b 68 61 70 70 79`


## Demo ##
[View video](http://instagram.com/p/jf3HmdQwsb/embed/#)