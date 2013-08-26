#include "ExampleAIModule.h"
#include <iostream>

using namespace BWAPI;
using namespace Filter;

void ExampleAIModule::onStart()
{
  // Hello World!
  Broodwar->sendText("Hello world!");

  // Print the map name.
  // BWAPI returns std::string when retrieving a string, don't forget to add .c_str() when printing!
  Broodwar << "The map is " << Broodwar->mapName() << "!" << std::endl;

  // Enable the UserInput flag, which allows us to control the bot and type messages.
  Broodwar->enableFlag(Flag::UserInput);

  // Uncomment the following line and the bot will know about everything through the fog of war (cheat).
  //Broodwar->enableFlag(Flag::CompleteMapInformation);

  // Set the command optimization level so that common commands can be grouped
  // and reduce the bot's APM (Actions Per Minute).
  Broodwar->setCommandOptimizationLevel(2);

  // Check if this is a replay
  if ( Broodwar->isReplay() )
  {

    // Announce the players in the replay
    Broodwar << "The following players are in this replay:" << std::endl;
    
    // Iterate all the players in the game using a std:: iterator
    Playerset players = Broodwar->getPlayers();
    for(auto p = players.begin(); p != players.end(); ++p)
    {
      // Only print the player if they are not an observer
      if ( !p->isObserver() )
        Broodwar << p->getName() << ", playing as " << p->getRace() << std::endl;
    }

  }
  else // if this is not a replay
  {
    // Retrieve you and your enemy's races. enemy() will just return the first enemy.
    // If you wish to deal with multiple enemies then you must use enemies().
    if ( Broodwar->enemy() ) // First make sure there is an enemy
      Broodwar << "The matchup is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << std::endl;
  }

}

void ExampleAIModule::onEnd(bool isWinner)
{
  // Called when the game ends
  if ( isWinner )
  {
    // Log your win here!
  }
}

void ExampleAIModule::onFrame()
{
	static int queuePotential = 0;
	static int newSupply = 0;
	static int marines = 0;
	static int totalRax = 0;
  // Called once every game frame

  // Display the game frame rate as text in the upper left area of the screen
  //Broodwar->drawTextScreen(200, 0,  "FPS: %d", Broodwar->getFPS() );
  //Broodwar->drawTextScreen(200, 20, "Average FPS: %f", Broodwar->getAverageFPS() );
	Unitset myUnits = Broodwar->self()->getUnits();
	  int i = 10;
	for ( Unitset::iterator u = myUnits.begin(); u != myUnits.end(); ++u )
	  Broodwar->drawTextScreen(200, i+=10, "Order: %s", u->getOrder().getName().c_str() );

  // Return if the game is a replay or is paused
  if ( Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self() )
    return;

  // Prevent spamming by only running our onFrame once every number of latency frames.
  // Latency frames are the number of frames before commands are processed.
  if ( Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0 )
    return;

  if(newSupply == 0)
	  newSupply = Broodwar->self()->supplyTotal();
	//if supply check fails, then
	//set holdMinsForDepot, and nothing well build
  int oldQueuePotential = queuePotential;
  queuePotential = 0;
  int oldMarines = marines;
  marines = 0;
  bool alreadyMakingRax = false;
  // Iterate through all the units that we own
  //Unitset myUnits = Broodwar->self()->getUnits();
  bool lazytemp = false;
  for ( Unitset::iterator u = myUnits.begin(); u != myUnits.end(); ++u )
  {
    // Ignore the unit if it no longer exists
    // Make sure to include this block when handling any Unit pointer!
    if ( !u->exists() )
      continue;

    // Ignore the unit if it has one of the following status ailments
    if ( u->isLockedDown() || u->isMaelstrommed() || u->isStasised() )
      continue;

    // Ignore the unit if it is in one of the following states
    if ( u->isLoaded() || u->isUnpowered() || u->isStuck() )
      continue;

    // Ignore the unit if it is incomplete or busy constructing
    if ( !u->isCompleted() || u->isConstructing() )
      continue;

	UnitType rax = BWAPI::UnitTypes::Terran_Barracks; //convinience and necessary when padding it to a lambda function
	UnitType depot = BWAPI::UnitTypes::Terran_Supply_Depot; //another convinience

	//check supply, and if we're about to be blocked, then set a flag to stop production.
	bool needSupply = false;
	if(Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed() <= (oldQueuePotential * 2) //+1 is just to help against supply blocks a bit more.
			&& Broodwar->self()->supplyTotal() == newSupply) {
		//Broodwar << "Waiting to start building a supply depot..." << std::endl;
		needSupply = true;
	}

	    // Finally make the unit do some stuff!

    // If the unit is a worker unit
    if ( u->getType().isWorker() )
	{
	  		//if we need to build a supply depot??
		if(needSupply && Broodwar->self()->minerals() >= 150 /*Broodwar->canMake(depot, NULL)*/) //broken till i use events [after events, use canMake]
		{
			Broodwar << "finding location for a Supply Depot" << std::endl;
			TilePosition targetBuildLocation = Broodwar->getBuildLocation(depot, u->getTilePosition());
			if ( targetBuildLocation )
			{
				// Register an event that draws the target build location
				Broodwar << "Attempting... Building a Supply Depot" << std::endl;

				// Order the builder to construct the supply structure
				if(u->build( depot, targetBuildLocation ))
				{
									Broodwar->registerEvent([targetBuildLocation, depot](Game*)
                                       {
                                         Broodwar->drawBoxMap( Position(targetBuildLocation),
                                                               Position(targetBuildLocation + depot.tileSize()),
                                                               Colors::Blue);
                                       },
                                       nullptr,  // condition
                                       depot.buildTime() + 100 );  // frames to run
					newSupply = Broodwar->self()->supplyTotal() + 16;	//a hack to say we are building a supply depot, no worries.
				}
				else
				{
					Broodwar << "Failed: Building a supply depot" << std::endl;
					// If the call fails, then print the last error message
					Broodwar << Broodwar->getLastError() << std::endl;
				}
			}
		}
				//this is entirely based on being terran.
		else if (!needSupply && totalRax < 7 && Broodwar->self()->supplyUsed() >= (((6+totalRax) * totalRax) + 20) && Broodwar->self()->minerals() >= 200 /*Broodwar->canMake(rax, *u)*/)
		{
			Broodwar << "Supply used: " << Broodwar->self()->supplyUsed() << std::endl;
			TilePosition targetBuildLocation = Broodwar->getBuildLocation(rax, u->getTilePosition());
			if ( targetBuildLocation )
			{
				// Order the builder to construct the rax structure
				if(u->build( rax, targetBuildLocation ))
				{
									// Register an event that draws the target build location
					Broodwar << "Building a Barracks" << std::endl;
					Broodwar->registerEvent([targetBuildLocation, rax](Game*)
                                       {
                                         Broodwar->drawBoxMap( Position(targetBuildLocation),
                                                               Position(targetBuildLocation + rax.tileSize()),
                                                               Colors::Blue);
                                       },
                                       nullptr,  // condition
                                       rax.buildTime() + 100 );  // frames to run
					++totalRax;
					Broodwar << "totalRax supply value: " << ((totalRax * 10) + 11) << std::endl;
				}
			}
		}
      // if our worker is idle
      else if ( u->isIdle() )
      {
        // Order workers carrying a resource to return them to the center,
        // otherwise find a mineral patch to harvest.
        if ( u->isCarryingGas() || u->isCarryingMinerals() )
        {
          u->returnCargo();
        }
        //else if ( !u->getPowerUp() )  // The worker cannot harvest anything if it
        //{                             // is carrying a powerup such as a flag
          // Harvest from the nearest mineral patch or gas refinery
          else if ( !u->gather( u->getClosestUnit( IsMineralField || IsRefinery )) )
          {
            // If the call fails, then print the last error message
            Broodwar << Broodwar->getLastError() << std::endl;
          }
		  else if(needSupply)
			  Broodwar << Broodwar->getLastError() << std::endl;

        //} // closure: has no powerup
      } // closure: if idle

    }
	else if ( u->getType() == rax)
	{
		++queuePotential;
		if(!needSupply && u->isIdle() && u->canTrain(UnitTypes::Terran_Marine))
		{
			u->train(UnitTypes::Terran_Marine);
			Broodwar << "Building a Marine" << std::endl;
			Broodwar << "oldQueuePotential: " << oldQueuePotential << std::endl;
			++marines;
		}
	}
    else if ( u->getType().isResourceDepot() ) // A resource depot is a Command Center, Nexus, or Hatchery
    {
		++queuePotential;
      // Order the depot to construct more workers! But only when it is idle.
      if ( !needSupply && Broodwar->self()->supplyUsed() < 60 && u->isIdle() && !u->train(u->getType().getRace().getWorker()))
      {
        // If that fails, draw the error at the location so that you can visibly see what went wrong!
        // However, drawing the error once will only appear for a single frame
        // so create an event that keeps it on the screen for some frames
        Position pos = u->getPosition();
        Error lastErr = Broodwar->getLastError();
        Broodwar->registerEvent([pos,lastErr](Game*){ Broodwar->drawTextMap(pos, "%c%s", Text::White, lastErr.c_str()); },   // action
                                nullptr,    // condition
                                Broodwar->getLatencyFrames());  // frames to run
      } // closure: failed to train idle unit

    }
	else if(oldMarines >= totalRax*4 && u->getType().canMove() && u->isIdle())
	{
		TilePosition::set starts = Broodwar->getStartLocations();
		for(TilePosition::set::iterator it = starts.begin(); it != starts.end(); ++it)
		{
			if(*it == Broodwar->self()->getStartLocation())
				continue;

			BWAPI::TilePosition start = *it;
			u->attack(Position(start.x * TILE_SIZE, start.y * TILE_SIZE));
			break;
		}
	}
	else if(oldMarines < 12 && u->getType().canMove() && u->isIdle())
	{
		++marines;
	}

  } // closure: unit iterator
}

void ExampleAIModule::onSendText(std::string text)
{

  // Send the text to the game if it is not being processed.
  Broodwar->sendText("%s", text.c_str());


  // Make sure to use %s and pass the text as a parameter,
  // otherwise you may run into problems when you use the %(percent) character!

}

void ExampleAIModule::onReceiveText(BWAPI::Player* player, std::string text)
{
  // Parse the received text
  Broodwar << player->getName() << " said \"" << text << "\"" << std::endl;
}

void ExampleAIModule::onPlayerLeft(BWAPI::Player* player)
{
  // Interact verbally with the other players in the game by
  // announcing that the other player has left.
  Broodwar->sendText("Goodbye %s!", player->getName().c_str());
}

void ExampleAIModule::onNukeDetect(BWAPI::Position target)
{

  // Check if the target is a valid position
  if ( target )
  {
    // if so, print the location of the nuclear strike target
    Broodwar << "Nuclear Launch Detected at " << target << std::endl;
  }
  else 
  {
    // Otherwise, ask other players where the nuke is!
    Broodwar->sendText("Where's the nuke?");
  }

  // You can also retrieve all the nuclear missile targets using Broodwar->getNukeDots()!
}

void ExampleAIModule::onUnitDiscover(BWAPI::Unit* unit)
{
}

void ExampleAIModule::onUnitEvade(BWAPI::Unit* unit)
{
}

void ExampleAIModule::onUnitShow(BWAPI::Unit* unit)
{
}

void ExampleAIModule::onUnitHide(BWAPI::Unit* unit)
{
}

void ExampleAIModule::onUnitCreate(BWAPI::Unit* unit)
{
  if ( Broodwar->isReplay() )
  {
    // if we are in a replay, then we will print out the build order of the structures
    if ( unit->getType().isBuilding() && !unit->getPlayer()->isNeutral() )
    {
      int seconds = Broodwar->getFrameCount()/24;
      int minutes = seconds/60;
      seconds %= 60;
      Broodwar->sendText("%.2d:%.2d: %s creates a %s", minutes, seconds, unit->getPlayer()->getName().c_str(), unit->getType().c_str());
    }
  }
}

void ExampleAIModule::onUnitDestroy(BWAPI::Unit* unit)
{
}

void ExampleAIModule::onUnitMorph(BWAPI::Unit* unit)
{
  if ( Broodwar->isReplay() )
  {
    // if we are in a replay, then we will print out the build order of the structures
    if ( unit->getType().isBuilding() && !unit->getPlayer()->isNeutral() )
    {
      int seconds = Broodwar->getFrameCount()/24;
      int minutes = seconds/60;
      seconds %= 60;
      Broodwar->sendText("%.2d:%.2d: %s morphs a %s", minutes, seconds, unit->getPlayer()->getName().c_str(), unit->getType().c_str());
    }
  }
}

void ExampleAIModule::onUnitRenegade(BWAPI::Unit* unit)
{
}

void ExampleAIModule::onSaveGame(std::string gameName)
{
  Broodwar << "The game was saved to \"" << gameName << "\"" << std::endl;
}

void ExampleAIModule::onUnitComplete(BWAPI::Unit *unit)
{
}
