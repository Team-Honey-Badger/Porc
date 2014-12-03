#ifndef __GameApplication_h_
#define __GameApplication_h_

#include "BaseApplication.h"
#include "Agent.h"

class GameApplication : public BaseApplication
{
private:
	Agent* player; //player
	Agent* agent; // a agent
	std::list<Agent*> agentList; //the agentList (holds the ghosts and the player)
	//grid size variables
	int x;
	int z;
	bool toggle;

public:
    GameApplication(void);
    virtual ~GameApplication(void);

	void loadEnv(std::string fileName);			// Load the buildings or ground plane, etc.
	void setupEnv();		// Set up the lights, shadows, etc
	void loadObjects();		// Load other props or objects (e.g. furniture)
	void loadCharacters();	// Load actors, agents, characters

	void addTime(Ogre::Real deltaTime);		// update the game state

	//////////////////////////////////////////////////////////////////////////
	// Lecture 4: keyboard interaction
	// moved from base application
	// OIS::KeyListener
    bool keyPressed( const OIS::KeyEvent &arg );
    bool keyReleased( const OIS::KeyEvent &arg );
    // OIS::MouseListener
    bool mouseMoved( const OIS::MouseEvent &arg );
    bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
    bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
	////////////////////////////////////////////////////////////////////////////

	Ogre::AxisAlignedBox targetHitBox; //barrel's hit box


protected:
    virtual void createScene(void);
};

#endif // #ifndef
