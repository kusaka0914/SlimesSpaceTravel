#pragma once

class Boat;
class Player;
class PlayerMovement;
class PlayerRespawn;

class PlayerBoatRide {
public:
    void Update(Player& player, PlayerMovement& movement, PlayerRespawn& respawn);
    void FollowMovingBoat(Player& player, Boat* boat) const;
    bool IsWaitingForBoat(const Player& player) const;
    bool CancelWaitingBoatRide(Player& player) const;
    bool IsTouchingBoat(const Player& player, Boat* boat) const;
    void StartRidingBoat(Player& player, Boat* boat) const;
    void OnBoatArrived(Player& player, PlayerMovement& movement, PlayerRespawn& respawn, Boat* boat) const;

private:
    Boat* FindWaitingBoat(const Player& player) const;
};
