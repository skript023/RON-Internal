#pragma once
#include "utility/joaat.hpp"

#include <optional>
#include <string>

namespace SDK
{
	class AActor;
	class APlayerCharacter;
}

namespace big::host_link
{
	// Whether the host of this lobby answered Check Host Mod with Accept Client Requests on.
	bool ready();

	// As a client: asks the host to run a mission action or a client action, or says it needs the host when the host has
	// not answered.
	void run(const std::string& name, const std::string& label, const std::string& args = {});

	// As a client: sends the host each change to a forwarded toggle. As the host: runs the toggles clients have on, on
	// their pawns. Called every update.
	void tick();

	// A toggle the host runs for you, so joining as a client leaves it on while ready().
	bool forwards(joaat_t toggle);

	// Noclip as a client: the game's own movement flies the pawn without collision, on your game and on the host's copy
	// of your pawn alike, so the host does not pull you back. speed is Noclip Speed.
	void fly(SDK::APlayerCharacter* character, float speed);
	void land(SDK::APlayerCharacter* character);

	using apply_t = void (*)(SDK::APlayerCharacter* character, float value);
	using remove_t = void (*)(SDK::APlayerCharacter* character);
	using value_t = float (*)();

	// A toggle of yours that works on one pawn. As a client its state, and value() while on, goes to the host, which
	// calls apply on your pawn every tick while it is on and remove once it goes off; either may be left out.
	struct client_toggle
	{
		client_toggle(joaat_t toggle, apply_t apply, remove_t remove = nullptr, value_t value = nullptr);
	};

	// As the host: the value a client sent with a toggle it has on, for the client whose pawn character is.
	std::optional<float> client_value(SDK::AActor* character, joaat_t toggle);

	using action_t = bool (*)(SDK::APlayerCharacter* character, const std::string& args);

	// Something the host does once for a client, with run(name, label, args) on the client; character is that client's
	// pawn, which may be null, and run returns whether it worked.
	struct client_action
	{
		client_action(const char* name, const char* label, action_t run);
	};
}
