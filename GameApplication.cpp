#include "GameApplication.h"
#include <fstream>
#include <sstream>
#include <map> 
#include "Grid.h"
#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream

//-------------------------------------------------------------------------------------
GameApplication::GameApplication(void)
{
	agent = NULL; // Init member data
	player = NULL;
	//grid = NULL;
	x = 0;
	z = 0;

	//start at level 1
	level = 1;
}
//-------------------------------------------------------------------------------------
GameApplication::~GameApplication(void)
{
	if (agent != NULL)  // clean up memory
		delete agent; 
}

//-------------------------------------------------------------------------------------
void GameApplication::createScene(void)
{
    loadEnv("map1.txt"); //changed loadEnv such that it can take a string as a parameter, and then will try to load that txt file
	setupEnv();
	loadObjects();
	loadCharacters();
}

void GameApplication::createGUI()
{
	if (mTrayMgr == NULL) return;
	using namespace OgreBites;

	//creates the Simple GUI element that displays your lives left
	Ogre::StringVector life;
	life.push_back("Lives Remaining");
	lifeBoard = mTrayMgr->createParamsPanel(OgreBites::TL_TOPLEFT,"Lives Remaining",320,life);

	mTrayMgr->showAll();
}

//////////////////////////////////////////////////////////////////
// Lecture 5: Returns a unique name for loaded objects and agents
std::string getNewName() // return a unique name 
{
	static int count = 0;	// keep counting the number of objects

	std::string s;
	std::stringstream out;	// a stream for outputing to a string
	out << count++;			// make the current count into a string
	s = out.str();

	return "object_" + s;	// append the current count onto the string
}

// Lecture 5: Load level from file!
void // Load the buildings or ground plane, etc
GameApplication::loadEnv(std::string fileName)
{
	//seed random
	srand(time(0));

	using namespace Ogre;	// use both namespaces
	using namespace std;

	class readEntity // need a structure for holding entities
	{
	public:
		string filename;
		float y;
		float scale;
		float orient;
		bool agent;
		char type;
	};

	////for saving goal coodinates
	//int Gi = -1, Gj = -1;

	ifstream inputfile;		// Holds a pointer into the file

	string path = __FILE__; //gets the current cpp file's path with the cpp file
	path = path.substr(0,1+path.find_last_of('\\')); //removes filename to leave path
	path+= fileName; //if txt file is in the same directory as cpp file
	inputfile.open(path);

	//inputfile.open("D:/CS425-2012/Lecture 8/GameEngine-loadLevel/level001.txt"); // bad explicit path!!!
	if (!inputfile.is_open()) // oops. there was a problem opening the file
	{
		cout << "ERROR, FILE COULD NOT BE OPENED" << std::endl;	// Hmm. No output?
		return;
	}

	// the file is open
	inputfile >> x >> z;	// read in the dimensions of the grid
	string matName;
	inputfile >> matName;	// read in the material name

	// create floor mesh using the dimension read
	MeshManager::getSingleton().createPlane("floor", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
		Plane(Vector3::UNIT_Y, 0), x*NODESIZE, z*NODESIZE, x, z, true, 1, x, z, Vector3::UNIT_Z);
	
	//create a floor entity, give it material, and place it at the origin
	Entity* floor = mSceneMgr->createEntity("Floor", "floor");
	floor->setMaterialName(matName);
	floor->setCastShadows(false);
	mSceneMgr->getRootSceneNode()->attachObject(floor);

	Grid *grid = new Grid(mSceneMgr, z, x); // Set up the grid. z is rows, x is columns
	
	string buf;
	inputfile >> buf;	// Start looking for the Objects section
	while  (buf != "Objects")
		inputfile >> buf;
	if (buf != "Objects")	// Oops, the file must not be formated correctly
	{
		cout << "ERROR: Level file error" << endl;
		return;
	}

	// read in the objects
	readEntity *rent = new readEntity();	// hold info for one object
	std::map<string,readEntity*> objs;		// hold all object and agent types;
	while (!inputfile.eof() && buf != "Characters") // read through until you find the Characters section
	{ 
		inputfile >> buf;			// read in the char
		if (buf != "Characters")
		{
			inputfile >> rent->filename >> rent->y >> rent->orient >> rent->scale;  // read the rest of the line
			rent->agent = false;		// these are objects
			objs[buf] = rent;			// store this object in the map
			rent = new readEntity();	// create a new instance to store the next object
		}
	}

	while  (buf != "Characters")	// get through any junk
		inputfile >> buf;
	
	// Read in the characters
	while (!inputfile.eof() && buf != "World") // Read through until the world section
	{
		inputfile >> buf;		// read in the char
		if (buf != "World")
		{
			inputfile >> rent->filename >> rent->y >> rent->scale >> rent->type; // read the rest of the line
			rent->agent = true;			// this is an agent
			objs[buf] = rent;			// store the agent in the map
			rent = new readEntity();	// create a new instance to store the next object
		}
	}
	delete rent; // we didn't need the last one

	// read through the placement map
	char c;
	for (int i = 0; i < z; i++)			// down (row)
		for (int j = 0; j < x; j++)		// across (column)
		{
			inputfile >> c;			// read one char at a time
			buf = c + '\0';			// convert char to string
			rent = objs[buf];		// find cooresponding object or agent
			if (rent != NULL)		// it might not be an agent or object
				if (rent->agent)	// if it is an agent...
				{
					if (rent->type == 'c')
					{
						agent = new Agent(this->mSceneMgr, getNewName(), rent->filename, rent->y, rent->scale, grid, rent->type);
						agentList.push_back(agent);
						agent->setPosition(grid->getPosition(i,j).x, rent->y, grid->getPosition(i,j).z);
						agent->setSelfNode(i, j);
						agent->setStartNode(i, j);
						player = agent;
					}
					if (rent->type == 'g')
					{
						agent = new Agent(this->mSceneMgr, getNewName(), rent->filename, rent->y, rent->scale, grid, rent->type);
						agentList.push_back(agent);
						agent->setPosition(grid->getPosition(i,j).x, rent->y, grid->getPosition(i,j).z);
						agent->setSelfNode(i, j);
						agent->setStartNode(i, j);
					}
					if (rent->type == 'p')
					{
						agent = new Agent(this->mSceneMgr, getNewName(), rent->filename, rent->y, rent->scale, grid, rent->type);
						agentList.push_back(agent);
						agent->setPosition(grid->getPosition(i,j).x, rent->y, grid->getPosition(i,j).z);
						agent->setSelfNode(i, j);
						agent->setStartNode(i, j);
					}
					if (rent->type == 'x')
					{
						agent = new Agent(this->mSceneMgr, getNewName(), rent->filename, rent->y, rent->scale, grid, rent->type);
						agentList.push_back(agent);
						agent->setPosition(grid->getPosition(i,j).x, rent->y, grid->getPosition(i,j).z);
						agent->setSelfNode(i, j);
						agent->setStartNode(i, j);
						grid->getNode(i,j)->setID(2);
					}
				}
				else	// Load objects
				{
					grid->loadObject(getNewName(), rent->filename, i, rent->y, j, rent->scale);
					
				}
			else // not an object or agent
			{
				if (c == 'w') // create a wall
				{
					Entity* ent = mSceneMgr->createEntity(getNewName(), Ogre::SceneManager::PT_CUBE);
					ent->setMaterialName("WoodPallet");
					Ogre::SceneNode* mNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
					mNode->attachObject(ent);
					mNode->scale(0.1f,0.2f,0.1f); // cube is 100 x 100
					grid->getNode(i,j)->setOccupied();  // indicate that agents can't pass through
					mNode->setPosition(grid->getPosition(i,j).x, 10.0f, grid->getPosition(i,j).z);
				}
				else if (c == 'e')
				{
					ParticleSystem::setDefaultNonVisibleUpdateTimeout(5);  // set nonvisible timeout
					ParticleSystem* ps = mSceneMgr->createParticleSystem(getNewName(), "Examples/PurpleFountain");
					Ogre::SceneNode* mNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
					mNode->attachObject(ps);
					mNode->setPosition(grid->getPosition(i,j).x, 0.0f, grid->getPosition(i,j).z);
				}
			}
		}
	
	// delete all of the readEntities in the objs map
	rent = objs["s"]; // just so we can see what is going on in memory (delete this later)
	
	std::map<string,readEntity*>::iterator it;
	for (it = objs.begin(); it != objs.end(); it++) // iterate through the map
	{
		delete (*it).second; // delete each readEntity
	}
	objs.clear(); // calls their destructors if there are any. (not good enough)
	
	inputfile.close();

	//give a pointer of the player agent to all other agents
	for(std::list<Agent*>::iterator i = agentList.begin(); i != agentList.end(); i++){
		(*i)->setPlayer(player);
	}

}

void // Set up lights, shadows, etc
GameApplication::setupEnv()
{
	using namespace Ogre;

	// set shadow properties
	mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_MODULATIVE);
	mSceneMgr->setShadowColour(ColourValue(0.5, 0.5, 0.5));
	mSceneMgr->setShadowTextureSize(1024);
	mSceneMgr->setShadowTextureCount(1);

	// use small amount of ambient lighting
	mSceneMgr->setAmbientLight(ColourValue(0.3f, 0.3f, 0.3f));

	// add a bright light above the scene
	Light* light = mSceneMgr->createLight();
	light->setType(Light::LT_POINT);
	light->setPosition(-10, 40, 20);
	light->setSpecularColour(ColourValue::White);
}

void // Load other props or objects
GameApplication::loadObjects()
{
	//nothing
}

void // Load actors, agents, characters
GameApplication::loadCharacters()
{
	//nothing
}

void
GameApplication::addTime(Ogre::Real deltaTime)
{
	// Iterate over the list of agents
	for (std::list<Agent*>::iterator iter = agentList.begin(); iter != agentList.end(); iter++)
		if (*iter != NULL)
			(*iter)->update(deltaTime);

	if(player) //if the agent being updated is the player character
	{
		//look for win/lose conditions that result in changing levels
		levelManager();

		//convert ints to hearts
		switch(player->getLives()){
		case 3:
			lifeBoard->setParamValue(0, "<3\t<3\t<3\t");
			break;
		case 2:
			lifeBoard->setParamValue(0, "<3\t<3\t\t");
			break;
		case 1:
			lifeBoard->setParamValue(0, "<3\t\t\t");
			break;
		default:
			lifeBoard->setParamValue(0, "You are dead :(");
		}
	}
	else{
		lifeBoard->setParamValue(0, "Who cares, you won!");
	}
}

bool 
GameApplication::keyPressed( const OIS::KeyEvent &arg ) // Moved from BaseApplication
{
    if (mTrayMgr->isDialogVisible()) return true;   // don't process any more keys if dialog is up

    if (arg.key == OIS::KC_F)   // toggle visibility of advanced frame stats
    {
        mTrayMgr->toggleAdvancedFrameStats();
    }
    else if (arg.key == OIS::KC_G)   // toggle visibility of even rarer debugging details
    {
        if (mDetailsPanel->getTrayLocation() == OgreBites::TL_NONE)
        {
            mTrayMgr->moveWidgetToTray(mDetailsPanel, OgreBites::TL_TOPRIGHT, 0);
            mDetailsPanel->show();
        }
        else
        {
            mTrayMgr->removeWidgetFromTray(mDetailsPanel);
            mDetailsPanel->hide();
        }
    }
    else if (arg.key == OIS::KC_T)   // cycle polygon rendering mode
    {
        Ogre::String newVal;
        Ogre::TextureFilterOptions tfo;
        unsigned int aniso;

        switch (mDetailsPanel->getParamValue(9).asUTF8()[0])
        {
        case 'B':
            newVal = "Trilinear";
            tfo = Ogre::TFO_TRILINEAR;
            aniso = 1;
            break;
        case 'T':
            newVal = "Anisotropic";
            tfo = Ogre::TFO_ANISOTROPIC;
            aniso = 8;
            break;
        case 'A':
            newVal = "None";
            tfo = Ogre::TFO_NONE;
            aniso = 1;
            break;
        default:
            newVal = "Bilinear";
            tfo = Ogre::TFO_BILINEAR;
            aniso = 1;
        }

        Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering(tfo);
        Ogre::MaterialManager::getSingleton().setDefaultAnisotropy(aniso);
        mDetailsPanel->setParamValue(9, newVal);
    }
    else if (arg.key == OIS::KC_R)   // cycle polygon rendering mode
    {
        Ogre::String newVal;
        Ogre::PolygonMode pm;

        switch (mCamera->getPolygonMode())
        {
        case Ogre::PM_SOLID:
            newVal = "Wireframe";
            pm = Ogre::PM_WIREFRAME;
            break;
        case Ogre::PM_WIREFRAME:
            newVal = "Points";
            pm = Ogre::PM_POINTS;
            break;
        default:
            newVal = "Solid";
            pm = Ogre::PM_SOLID;
        }

        mCamera->setPolygonMode(pm);
        mDetailsPanel->setParamValue(10, newVal);
    }
    else if(arg.key == OIS::KC_F5)   // refresh all textures
    {
        Ogre::TextureManager::getSingleton().reloadAll();
    }
    else if (arg.key == OIS::KC_SYSRQ)   // take a screenshot
    {
        mWindow->writeContentsToTimestampedFile("screenshot", ".jpg");
    }
    else if (arg.key == OIS::KC_ESCAPE)
    {
        mShutDown = true;
    }
	else if (arg.key == OIS::KC_1)				//level 1
	{
		level = 1;							//identify level
		mSceneMgr->destroyAllEntities();	//erase all things in the level
		agentList.clear();					//erase all the agents in the level
		deleteAgentList();					//free memory
		loadEnv("map1.txt");				//load a level
	}
	else if (arg.key == OIS::KC_2)				//level 2
	{
		level = 2;							
		mSceneMgr->destroyAllEntities();
		agentList.clear();
		deleteAgentList();					//free memory
		loadEnv("map2.txt");
	}
	else if (arg.key == OIS::KC_3)				//level 3
	{
		level = 3;							
		mSceneMgr->destroyAllEntities();
		agentList.clear();
		deleteAgentList();					//free memory
		loadEnv("map3.txt");
	}
	else if (arg.key == OIS::KC_4)				//level 4
	{
		level = 4;							
		mSceneMgr->destroyAllEntities();
		agentList.clear();
		deleteAgentList();					//free memory
		loadEnv("map4.txt");
	}
	else if (arg.key == OIS::KC_W || arg.key == OIS::KC_UP)
	{
		if(player)
			player->setOrientation(1);
	}
	else if (arg.key == OIS::KC_S || arg.key == OIS::KC_DOWN)
	{
		if(player)
			player->setOrientation(2);
	}
	else if (arg.key == OIS::KC_D || arg.key == OIS::KC_RIGHT)
	{
		if(player)
			player->setOrientation(3);
	}
	else if (arg.key == OIS::KC_A || arg.key == OIS::KC_LEFT)
	{
		if(player)
			player->setOrientation(4);
	}

    return true;
}

bool GameApplication::keyReleased( const OIS::KeyEvent &arg )
{
    return true;
}

bool GameApplication::mouseMoved( const OIS::MouseEvent &arg )
{
    if (mTrayMgr->injectMouseMove(arg)) return true;
    return true;
}

bool GameApplication::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
    if (mTrayMgr->injectMouseDown(arg, id)) return true;
    return true;
}

bool GameApplication::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
    if (mTrayMgr->injectMouseUp(arg, id)) return true;
    return true;
}

void GameApplication::levelManager(){
	if(player->reset || player->doneWithLevel)
	{
		if(player->reset)						//if level needs to be reset
		{										
			level = 1;							//return to level 1
		}
		else if(player->doneWithLevel)			//if player wins a level
		{
			level++;							//go to next level
		}
		mSceneMgr->destroyAllEntities();		//erase all the things in the level
		agentList.clear();						//erase all the agents
		deleteAgentList();						//free memory
		std::stringstream levelName;			//build the map name into here
		levelName << "map" << level << ".txt";	//write the file name with the level number
		std::string map;						//write the map name into here 
		levelName >> map;						//stream built file name into map string
		//std::cout<<map<<std::endl;				
		loadEnv(map);							//load map							
		player->reset = false;					//the level no longer needs to be reset
		player->doneWithLevel = false;			//the player is no longer done with the level
	}
}

void GameApplication::deleteAgentList(){
	for(std::list<Agent*>::iterator i = agentList.begin(); i != agentList.end(); i++){
		delete (*i);
		(*i) = NULL;
	}
}