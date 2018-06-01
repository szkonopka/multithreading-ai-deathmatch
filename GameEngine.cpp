#include "GameEngine.h"

void GameEngine::Redraw()
{
  gr.playerResource.lock();
  float percentageA = teamABar->getCurrentProgress();
  float percentageB = teamBBar->getCurrentProgress();
  teamABar->draw(Players::TeamAName, percentageA);
  teamBBar->draw(Players::TeamBName, percentageB);

  for(Player *item : teamA)
  {
    item->draw();
  }

  for(Player *item : teamB)
  {
    item->draw();
  }

  gr.playerResource.unlock();
}

void GameEngine::Refresh()
{
  gr.playerResource.lock();
  teamASumHP = 0;
  teamBSumHP = 0;

  for(Player *player : teamA)
  {
    teamASumHP += player->getHp();
  }

  for(Player *player : teamB)
  {
    teamBSumHP += player->getHp();
  }

  gameOn = (teamASumHP <= 0 || teamBSumHP <= 0) ? false : true;
  GameState::GameOnPtr = &gameOn;
  gr.playerResource.unlock();

  teamABar->setCurrentProgress(teamASumHP / teamATotalHP);
  teamBBar->setCurrentProgress(teamBSumHP / teamBTotalHP);
}

void GameEngine::Run()
{
  Players::PlayerID = 0;
  color3 teamAColor = { 0.75f, 0.1f, 0.25f };
  color3 teamBColor = { 0.15f, 0.3f, 0.75f };

  Armory *teamAArmory = new Armory();
  Armory *teamBArmory = new Armory();

  for(int i = 0; i < teamSize; i++)
  {
    float yStep = ((-matrixWidth / 2) + (i * matrixWidth / 2));
    teamA.push_back(new Player(-matrixWidth / 2, yStep, 100, 100, 0, teamAColor, teamAArmory));
    teamB.push_back(new Player(matrixWidth / 2, yStep, 100, 100, 0, teamBColor, teamBArmory));
  }

  for(int i = 0; i < teamSize; i++)
  {
    teamATotalHP += teamA[i]->getMaxHp();
    teamBTotalHP += teamB[i]->getMaxHp();
  }

  std::cout << teamASumHP << std::endl;
  std::cout << teamBSumHP << std::endl;

  for(int i = 0; i < teamSize; i++)
  {
    std::cout << "A[" << i << "] X: " << teamA[i]->getXPosition() << ", Y: " << teamA[i]->getYPosition() << std::endl;
    std::cout << "B[" << i << "] X: " << teamB[i]->getXPosition() << ", Y: " << teamB[i]->getYPosition() << std::endl;
  }

  teamA[0]->setWeapon(teamAArmory->getSniper());
  teamA[1]->setWeapon(teamAArmory->getPistol());
  teamA[2]->setWeapon(teamAArmory->getShotgun());
  teamB[0]->setWeapon(teamBArmory->getSniper());
  teamB[1]->setWeapon(teamBArmory->getPistol());
  teamB[2]->setWeapon(teamBArmory->getShotgun());


  std::thread *teamAThreads = new std::thread[teamSize];
  std::thread *teamBThreads = new std::thread[teamSize];
  std::thread shootTickRate(&GameEngine::ShootTickRate, this);
  std::thread checkBulletsCollision(&GameEngine::CheckBulletsCollision, this);

  for(int i = 0; i < teamSize; i++)
  {
    teamAThreads[i] = std::thread(&Player::play, teamA[i]);
    teamBThreads[i] = std::thread(&Player::play, teamB[i]);
  }

  for(int i = 0; i < teamSize; i++)
  {
    teamAThreads[i].join();
    teamBThreads[i].join();
  }

  checkBulletsCollision.join();
  shootTickRate.join();
}

void GameEngine::ShootTickRate()
{
  while(true)
  {
    if(!gameOn)
      break;

    Players::IsShooting = true;
    CheckInWeaponsRange();
  }
}

void GameEngine::CheckInWeaponsRange()
{
  while(true)
  {
    Player *shooter = nullptr;
    if(!gameOn)
      break;

    for(Player *playerA: teamA)
    {
      float ax = playerA->getXPosition();
      float ay = playerA->getYPosition();
      float aw = playerA->getXSize();
      float ar = playerA->getRange();

      for(Player *playerB : teamB)
      {
        float bx = playerB->getXPosition();
        float by = playerB->getYPosition();
        float bw = playerB->getXSize();
        float br = playerB->getRange();

        if(((ax + ar) >= (bx - bw)) && ((ax - ar) <= (bx + bw)))
        {
          if(((ay + ar) >= (by - bw)) && ((ay - ar) <= (by + bw)))
          {
            if(Players::IsShooting)
            {
              ShootInWeaponsRange(ax, bx, ay, by, playerA);
              shooter = playerA;
              //SLEEP(shooter->getCurrentWeapon()->getFrequency());
            }
          }
        }

        if(((bx + br) >= (ax - aw)) && ((bx - br) <= (ax + aw)))
        {
          if(((by + br) >= (ay - aw)) && ((by - br) <= (ay + aw)))
          {
            if(Players::IsShooting)
            {
              ShootInWeaponsRange(bx, ax, by, ay, playerB);
              shooter = playerB;
              //SLEEP(shooter->getCurrentWeapon()->getFrequency());
            }
          }
        }
      }
    }

    if(shooter != nullptr)
    {
      //gr.weaponResource.lock();
      SLEEP(shooter->getCurrentWeapon()->getFrequency());
      //gr.weaponResource.unlock();
    }
  }
}

void GameEngine::ShootInWeaponsRange(float ax, float bx, float ay, float by, Player *&shooter)
{
  float destX, destY;
  if(ax >= bx && ay >= by)
  {
    destX = bx - matrixWidth;
    destY = bx - matrixWidth;
  }

  if(ax >= bx && ay <= by)
  {
    destX = bx - matrixWidth;
    destY = bx + matrixWidth;
  }

  if(ax <= bx && ay >= by)
  {
    destX = bx + matrixWidth;
    destY = bx + matrixWidth;
  }

  if(ax <= bx && ay <= by)
  {
    destX = bx + matrixWidth;
    destY = bx - matrixWidth;
  }

  if(shooter->getCurrentWeapon()->getArmored())
    shooter->shoot(gr.bulletResource, gr.weaponResource, bx, by);
  //shooter->shoot(gr.bulletResource, destX, destY);
}

void GameEngine::CheckBulletsCollision()
{
  while(true)
  {
    if(!gameOn)
      break;
    CheckTeamCollision(teamA, teamB);
    CheckTeamCollision(teamB, teamA);
  }
}

void GameEngine::CheckTeamCollision(std::vector<Player *> &firstTeam, std::vector<Player *> &secondTeam)
{
  for(Player *playerA : firstTeam)
  {
    gr.bulletResource.lock();
    for(Bullet &bullet : playerA->firedBullets)
    {
      float bltx = bullet.getXPosition();
      float blty = bullet.getYPosition();

      for(Player *playerB : secondTeam)
      {
        float bx = playerB->getXPosition();
        float by = playerB->getYPosition();
        float bw = playerB->getXSize();

        if((bltx <= (bx + bw)) && (bltx >= (bx - bw)))
        {
          if((blty <= (by + bw)) && (blty >= (by -bw)))
          {
            gr.weaponResource.lock();
            if(playerA->getCurrentWeapon()->getArmored())
              playerB->setHp(playerB->getHp() - playerA->getCurrentWeapon()->getDamage());

            auto it = std::find(playerA->firedBullets.begin(), playerA->firedBullets.end(), bullet);
            if(it != playerA->firedBullets.end()) { playerA->firedBullets.erase(it); }

            if(playerB->getHp() <= 0.0f)
            {
              gr.playerResource.lock();
              playerB->setIsAlive(false);
              auto itPlayer = std::find(secondTeam.begin(), secondTeam.end(), playerB);
              if(itPlayer != secondTeam.end()) { secondTeam.erase(itPlayer); }
              gr.playerResource.unlock();
            }
            gr.weaponResource.unlock();
          }
        }
      }

      if(bltx < -(matrixWidth / 2) || bltx > (matrixWidth / 2) || blty < -(matrixWidth / 2) || blty > (matrixWidth / 2))
      {
        auto it = std::find(playerA->firedBullets.begin(), playerA->firedBullets.end(), bullet);
        if(it != playerA->firedBullets.end()) { playerA->firedBullets.erase(it); }
      }
    }
    gr.bulletResource.unlock();
  }
}
