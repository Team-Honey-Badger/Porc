Role Breakdown:


George:
Player movement
Collision detection
GUI
Did some basic framework stuff (like tweaking the level loader, etc)
Win condition
Camera setup
Thought of the final win state


Alex:
A* and AI for the ghosts
Level design
Barrels and their mechanics (collectables)
Lose conditions
“Art”/”Text Art”
Steering/Memory portion of player movement
Game flow (switching between maps)
Teleportation Tubes


How to play:


        Objectives:
                Avoid touching ghosts, they will follow you if they see you
                Collect all the yellow barrels to progress to next level
                Running out of lives resets back to level 
        
        Controls:
                WASD or Arrow keys for movement
                        Memory:
				when pressing a direction key, Porc will turn at the next intersection/turn node
			Grid-based:
				Porc must reach the center of a node before he can turn
                1,2,3,4 keys for level selection
                        Not recommended, but skipping levels is allowed


Requirements:
        
1. Player has full control over Porc and much help him survive
2. Game states: Playing game, Pause/Repositioning after losing lives, Winning Screen
3. Player and Ghosts play the run animation while moving and idle while standing
4. Ghosts move to random intersections/turn nodes within a range of 4
5. There are 4 levels that automatically load each time you pass the current level but,
reset back to level 1 when you run out of lives
1. Ghosts travel using A* towards near by turn nodes or to where the player is located
2. Each ghost checks for collision with the player
3. Barrels disappear when picked up and positions reset when losing lives, GUI shows lives as <3’s
4. Nothing like the other assignments
5. Challenging but not too hard once you get used to the controls
6. Didn’t use too many collisions, teleporting works correctly now (no more hacks), lots of comments
7. Files inside