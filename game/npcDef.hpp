#ifndef NPCDEF_HPP
# define NPCDEF_HPP

# include <string>
# include <vector>

# include "stats.hpp"

# define NPC_ROLE_ENEMY "enemy"

class NpcDef {
	private:
		std::string _id;
		std::string _name;
		std::string _role;
		std::string _description;
		std::vector<std::string> _dialogues;
		Stats _stats;
		std::vector<std::string> _loots;

	public:
		NpcDef(const std::string& id, const std::string& name, const std::string& role,
			const std::string& description, const std::vector<std::string>& dialogues,
			const Stats& stats, const std::vector<std::string>& loots);

		const std::string& id() const;
		const std::string& name() const;
		const std::string& role() const;
		const std::string& description() const;
		const std::string& dialogue() const;
		const Stats& stats() const;
};

#endif