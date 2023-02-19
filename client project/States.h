#pragma once
//so all class can access enums for states and packet types
enum class States { lobby, game, win };
enum class PacketTypes { PlayerData, ReadyToggle, StartGame, TimerSync, PlayerPositions, BulletFired, BulletCollision, GameOver};
