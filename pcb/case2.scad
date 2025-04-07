$fn=100;



module cables() {
  translate([75,60,0])
      rotate([0,0,90]) {


    // can cable
    color("red")
    translate([0,0,8]) 
    rotate([0,90,0]) {
        cylinder(d=6, h=50);
        //translate([-2.7,0,15])
        //cube([6,6,15], center=true);
    }
    
    // power
    color("green")
    translate([0,10,8])
    rotate([0,90,0]) {
        cylinder(d=4, h=50);
        //translate([-3,0,15])
        //cube([4,5.4,15], center=true);

    }


    //  ntc + fuel
      color("blue")
    translate([0,20,8])
    rotate([0,90,0]) {
        cylinder(d=5, h=50);
        //translate([-3,0,15])
        //cube([5,5,15], center=true);
    }
     
    //
      color("orange")
    translate([0,30,8])
    rotate([0,90,0]) {
        cylinder(d=4.5, h=50);
        //translate([-3.3,0,15])
        //cube([4.5,4.5,15], center=true);
    }


    //
       color("purple")
   translate([0,40,8])
    rotate([0,90,0]) {
        cylinder(d=4.5, h=50);
        //translate([-3.3,0,15])
        //cube([4.5,4.5,15], center=true);
    }


    // 1 wire
      color("yellow")
    translate([0,50,8])
    rotate([0,90,0]) {
        cylinder(d=4.5, h=50);
        //translate([-3.3,0,15])
        //cube([4.5,4.5,15], center=true);
    }



    }
}

module board() {
    
        translate([11,3,0]) 
        union() {
            cube([97,52,4]);
           // connectors
            color("blue") {
                // serial just for height
                translate([16,24,0])
                cube([12,3,11]);

                translate([6,7,0])
                cube([7.5,15,12]);

                translate([13.5,41,0])
                cube([10,7.5,12]);

                translate([25,41,0])
                cube([15,7.5,12]);

                translate([41,41,0])
                cube([10,7.5,12]);

                translate([52,41,0])
                cube([10,7.5,12]);

                translate([63,41,0])
                cube([10,7.5,12]);
                translate([76,41,0])
                cube([10,7.5,12]);
                translate([87,41,0])
                cube([10,7.5,12]);

                translate([81,16,0])
                cube([7.5,10,12]);

                translate([81,16,0])
                cube([7.5,10,12]);
                
                translate([84,2,0])
                cube([7.5,10,12]);
                

                
                // LED
                translate([42,12,0])
                            cylinder(h=25,d=2);
                translate([17,22,0])
                            cylinder(h=25,d=2);


            }
            cables();


        };
}



module lid(w=130,d=75,h=8,t=5) {
   difference() {
        translate([w/2,d/2-3,12]) 
      difference() {
        union() {
            difference() {
                union() {
                    cube([w-t*2,d,h], center=true);
                    cube([w,d-t*2,h], center=true);
                    translate([w/2-t,d/2-t,0])
                    cylinder(r=t,h=h, center=true);
                    translate([-(w/2-t),d/2-t,0])
                    cylinder(r=t,h=h, center=true);
                    translate([-(w/2-t),-(d/2-t),0])
                    cylinder(r=t,h=h, center=true);
                    translate([w/2-t,-(d/2-t),0])
                    cylinder(r=t,h=h, center=true);
                    

                }
                // inside
                translate([0,0,-8])
                    cube([w-t*2,d-t*2,h+t*2], center=true);
            }
            
            

            
            // fxings
            translate([w/2-t,d/2-t,0])
                    cylinder(r=5,h=h, center=true);
            translate([-(w/2-t),d/2-t,0])
                    cylinder(r=5,h=h, center=true);
            translate([-(w/2-t),-(d/2-t),0])
                    cylinder(r=5,h=h, center=true);
                    translate([w/2-t,-(d/2-t),0])
                    cylinder(r=5,h=h, center=true);
        }
        
            translate([w/2-t,d/2-t,0])
                    cylinder(d=3,h=h+10, center=true);
            translate([-(w/2-t),d/2-t,0])
                    cylinder(d=3,h=h+10, center=true);
            translate([-(w/2-t),-(d/2-t),0])
                    cylinder(d=3,h=h+10, center=true);
                    translate([w/2-t,-(d/2-t),0])
                    cylinder(d=3,h=h+10, center=true);
    }
    



        // led
  
        board();

    }
    
    


}

module case(w=130,d=75,h=12.5,t=5) {
   difference() {
   translate([w/2,d/2-3,2]) 
      difference() {
        union() {
            difference() {
                union() {
                    cube([w-t*2,d,h], center=true);
                    cube([w,d-t*2,h], center=true);
                    translate([w/2-t,d/2-t,0])
                    cylinder(r=t,h=h, center=true);
                    translate([-(w/2-t),d/2-t,0])
                    cylinder(r=t,h=h, center=true);
                    translate([-(w/2-t),-(d/2-t),0])
                    cylinder(r=t,h=h, center=true);
                    translate([w/2-t,-(d/2-t),0])
                    cylinder(r=t,h=h, center=true);
                    

                }
                // inside
                translate([0,0,t])
                    cube([w-t*2,d-t*2,h+t], center=true);
            }
            

            // fxings
            translate([w/2-t,d/2-t,0])
                    cylinder(r=5,h=h, center=true);
            translate([-(w/2-t),d/2-t,0])
                    cylinder(r=5,h=h, center=true);
            translate([-(w/2-t),-(d/2-t),0])
                    cylinder(r=5,h=h, center=true);
                    translate([w/2-t,-(d/2-t),0])
                    cylinder(r=5,h=h, center=true);
        }
        
        translate([w/2-t,d/2-t,0])
                cylinder(d=3,h=h+10, center=true);
        translate([-(w/2-t),d/2-t,0])
                cylinder(d=3,h=h+10, center=true);
        translate([-(w/2-t),-(d/2-t),0])
                cylinder(d=3,h=h+10, center=true);
        translate([w/2-t,-(d/2-t),0])
                cylinder(d=3,h=h+10, center=true);
    }
    board(); 
    }
}





difference() {
    union() {
        board();
        color("green") case();
        lid();
    }

   translate([0,0,0])
   cube([20,50,140], center=true);
}




//board();
//color("green") case();
//lid();
