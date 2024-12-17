$fn=100;



module cables() {
  translate([0,0,0]){

    // can cable
    translate([100,18,3]) 
    rotate([0,90,0]) {
    cylinder(d=6, h=50);
        translate([-2.7,0,15])
        cube([6,6,15], center=true);
    }
     

    // power
    translate([82,52,3])
    rotate([-90,0,0]) {
    cylinder(d=4, h=50);
                translate([0,-3,15])
        cube([4,5.4,15], center=true);

    }

    //  ntc
    translate([54,52,3])
    rotate([-90,0,0]) {
    cylinder(d=5, h=50);
                translate([0,-3,15])
        cube([5,5,15], center=true);
    }

    //
    translate([32,52,3])
    rotate([-90,0,0]) {
    cylinder(d=4.5, h=50);
                translate([0,-3.3,15])
        cube([4.5,4.5,15], center=true);
    }

    //
    translate([22,52,3])
    rotate([-90,0,0]) {
    cylinder(d=4.5, h=50);
                translate([0,-3.3,15])
        cube([4.5,4.5,15], center=true);
    }

    //
    translate([11,52,3])
    rotate([-90,0,0]) {
    cylinder(d=4.5, h=50);
                translate([0,-3.3,15])
        cube([4.5,4.5,15], center=true);
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
                translate([12,8,0])
                cube([3,22,11]);
                
                cables();

                
                // LED
                translate([51,31,0])
                            cylinder(h=25,d=2);


            }


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
    
    
        translate([11,3,0]) {
        // can cable
    translate([100,18,3]) 
    rotate([0,90,0]) {
        difference() {
        translate([-2.7,0,16.5])
        cube([6,5.8,5], center=true);
    cylinder(d=6, h=50);
        }
    }
     

    // power
    translate([82,52,3])
    rotate([-90,0,0]) {
        difference() {
                translate([0,-3,14.4])
        cube([3.8,5.4,t], center=true);
    cylinder(d=4, h=50);
        }

    }

    //  ntc
    translate([54,52,3])
    rotate([-90,0,0]) {
        difference() {
                translate([0,-3,14.4])
        cube([4.8,5,5], center=true);
    cylinder(d=5, h=50);
        }
    }

    //
    translate([32,52,3])
    rotate([-90,0,0]) {
        difference() {
                translate([0,-3.3,14.4])
        cube([4.3,4.5,5], center=true);
    cylinder(d=4.5, h=50);
        }
    }

    //
    translate([22,52,3])
    rotate([-90,0,0]) {
        difference() {
                translate([0,-3.3,14.4])
        cube([4.3,4.5,5], center=true);
    cylinder(d=4.5, h=50);
        }
    }

    //
    translate([11,52,3])
    rotate([-90,0,0]) {
        difference() {
                translate([0,-3.3,14.4])
        cube([4.3,4.5,5], center=true);
    cylinder(d=4.5, h=50);
         }
   }
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




/*
difference() {
    union() {
        //board();
        color("green") case();
        lid();
    }

   translate([0,0,0])
   cube([20,50,140], center=true);
}
*/



//board();
//color("green") case();
lid();
