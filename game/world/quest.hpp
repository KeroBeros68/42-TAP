#ifndef QUEST_HPP
# define QUEST_HPP

# include <string>


enum class QuestType {Deliver, Defeat, Fetch};

class Quest {

	private:
		std::string	_id;
		std::string	_name;
		QuestType	_type;
		std::string	_giver;
		std::string	_turn_in;
		std::string	_target;
		int			_count;
		std::string	_text_start;
		std::string	_text_done;

	public:
		Quest();
};

#endif