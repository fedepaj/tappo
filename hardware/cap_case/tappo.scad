$fn=100;
//Tank hole diameter
diam=42.6;
//Hose diam
hoseDiam=6.7;
//Air hole diameter
airOutDiam=3;
//Ultrasonic ears
sensorDiam=16.4;
//Distance between ears
sensorDistance=25.2;
//Switch length
switchL=20;
//Switch width
switchW=7;
//Switch Height
switchH=20;
//Sensor height
sensorH=10;
//altezza tappo
tappoH=15;
//Thickness of base
t=2;
//Borders thickness
thickness=15;
f=3;
module tappo(){
    difference(){
        union(){
            difference(){
                union(){
                    cylinder(d=diam+15,h=t);
                    cylinder(d1=diam,d2=diam-10,h=tappoH+t);
                    }
                translate([0,0,sensorH])cylinder(d=diam-thickness,h=tappoH-sensorH+t);
                translate([sensorDistance/2,0,0])cylinder(d=sensorDiam,h=tappoH+t);
                translate([-sensorDistance/2,0,0])cylinder(d=sensorDiam,h=tappoH+t);
            }
            translate([0,(diam-hoseDiam-thickness)/2,0])cylinder(d1=hoseDiam*2,d2=hoseDiam+f,h=tappoH+t);
            //translate([-55,(diam-airOutDiam-thickness)/2,0])cylinder(d=airOutDiam+f,h=tappoH+t);
        }
        translate([0,(diam-hoseDiam-thickness)/2,0])cylinder(d1=hoseDiam*2,d2=hoseDiam,h=tappoH+t);
        //translate([0,-(diam-airOutDiam-thickness)/2,0])cylinder(d=airOutDiam,h=tappoH+t);
        
        }
        /*difference() {
            cylinder(d1=diam+5.2,d2=diam-10+5.2,h=tappoH+t);
            cylinder(d1=diam+3,d2=diam-10+3,h=tappoH+t);
            translate([0,(diam+5.2)/8,(tappoH+t)/2])cube([diam+5.2,diam+5.2,tappoH+t],center=true);
            }*/
}
module tappoLevetta(){
    difference(){
        union(){
            tappo();
            translate([0,-(diam/2)+5+switchH/2,(tappoH+t)/2])cube([switchW+1.6,switchH-5,tappoH+t],center=true);
            }#translate([0,-(diam/2)+switchH/2,-switchL/2+tappoH])cube([switchW,switchH,switchL],center=true);
        }
        
}
tappoLevetta();
//tappo();