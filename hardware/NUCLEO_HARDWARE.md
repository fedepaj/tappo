#Hardware

## Ultrasonic range finder sr04
When the lid is on Is used to determine the distance between the water and the neck of the tank. 

The device gets a reference masure of the distance between the sensor and the level of the bottom of the tank or the level of already present fluid.

The difference between the reference vaule and the subsequent distance's samples defines the fill percentage that will be sended via the `measures` topic to the broker.

## The switch
The role of the switch is to decect when the lid has been placed on the neck of the tank and could be any sort of switch.

The switch chosen is [this one](http://smparts.com/product_info.php?cPath=2_602&products_id=6689),a lever switch with a little wheel on top of it.
Is placed in the circuit like [this](https://killerrobotics.files.wordpress.com/2015/09/lever-switch_bb.png).

The switch it raises an interruppt that publishes the activeness of the lid to the `active` topic and let the device stop measuring the fill percentage.

## Case
Is the one that holds the sensors together and makes it a perfect lid for a 43mm 30L plastic tank that will be filled with 15mm tubing.

## Piezo buzzer
Informs the user that the level of the water has reached the desired level with an acoustic signal.

## Led
Is surely a visual cue but could really be switched by a relay.