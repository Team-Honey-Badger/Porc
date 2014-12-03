#ifndef __Agent_h_
#define __Agent_h_

#include "BaseApplication.h"
#include <deque>

class Grid;
class GridNode;

class Agent
{
private:
	Ogre::SceneManager* mSceneMgr;		// pointer to scene graph
	Ogre::SceneNode* mBodyNode;			
	Ogre::Entity* mBodyEntity;
	float height;						// height the character should be moved up
	float scale;						// scale of character from original model
	char agentType;
	int orientation;					//determines the direction/orientation of the character
	int defaultOrientation;				//character keeps default orientation until able to adapt new one

	//player lives
	int lives;

	int score;

	Grid *grid; //pointer to the grid
	GridNode *selfNode; //where agent is right now
	GridNode *startNode;
	GridNode *goalNode;	//where agent wants to go
	GridNode *prev;		//where agent was last time
	bool toggle; //toggle between S and G
	Agent* player; //points to the player so ghosts can see him

	// all of the animations our character has, and a null ID
	// some of these affect separate body parts and will be blended together
	enum AnimID
	{
		ANIM_IDLE_BASE,
		ANIM_IDLE_TOP,
		ANIM_RUN_BASE,
		ANIM_RUN_TOP,
		ANIM_HANDS_CLOSED,
		ANIM_HANDS_RELAXED,
		ANIM_DRAW_SWORDS,
		ANIM_SLICE_VERTICAL,
		ANIM_SLICE_HORIZONTAL,
		ANIM_DANCE,
		ANIM_JUMP_START,
		ANIM_JUMP_LOOP,
		ANIM_JUMP_END,
		ANIM_NONE
	};

	Ogre::AnimationState* mAnims[13];		// master animation list
	AnimID mBaseAnimID;						// current base (full- or lower-body) animation
	AnimID mTopAnimID;						// current top (upper-body) animation
	bool mFadingIn[13];						// which animations are fading in
	bool mFadingOut[13];					// which animations are fading out
	Ogre::Real mTimer;						// general timer to see how long animations have been playing
	Ogre::Real mVerticalVelocity;			// for jumping

	void setupAnimations();					// load this character's animations
	void fadeAnimations(Ogre::Real deltaTime);				// blend from one animation to another
	void updateAnimations(Ogre::Real deltaTime);			// update the animation frame

	// for locomotion
	Ogre::Real mDistance;					// The distance the agent has left to travel
	Ogre::Vector3 mDirection;				// The direction the object is moving
	Ogre::Vector3 mDestination;				// The destination the object is moving towards
	std::deque<Ogre::Vector3> mWalkList;	// The list of points we are walking to
	Ogre::Real mWalkSpeed;					// The speed at which the object is moving
	bool nextLocation();					// Is there another destination?
	void updateLocomote(Ogre::Real deltaTime);			// update the character's walking

	std::vector<GridNode*> intersections;	// list of intersections

	void collide(Ogre::Real deltaTime);		//tests collision detection between player and ghosts

	//////////////////////////////////////////////
	// Lecture 4
	bool procedural;						// Is this character performing a procedural animation
    //////////////////////////////////////////////

	//pointer to the target's hitbox
	Ogre::AxisAlignedBox *targetHitBox;

public:
	Agent(Ogre::SceneManager* SceneManager, std::string name, std::string filename, float height, float scale, Grid *grid, char type/*, Agent* player*/);
	~Agent();

	bool isFirstPosSet; //is the first position set, used for setting up initial position

	void setPosition(float x, float y, float z);

	void setOrientation(int orientation);
	int getOrientation();

	char getAgentType();

	void update(Ogre::Real deltaTime);		// update the agent
	
	void setBaseAnimation(AnimID id, bool reset = false);	// choose animation to display
	void setTopAnimation(AnimID id, bool reset = false);

	void moveTo(); //move to a new position

	int getScore(); //returns the score of the game (needed for scoreboard)
	int getLives(); //returns the number of the lives the player has (needed for scoreboard)

	//setters for GridNodes
	void setSelfNode(int r, int c);
	GridNode* getSelfNode();
	void setStartNode(int r, int c);
	void setGoalNode();
	void setPlayer(Agent* player);

	void loseLife();	//subtract lives

	//reset after losing a life
	bool reset;

	//progress to next level or win
	bool doneWithLevel;


};

#endif