//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "quest.hpp"

//##############################################################################
//#                                  QUEST DEF                                 #
//##############################################################################

//######################## CONSTRUCTOR #########################

QuestDef::QuestDef(const std::string& id, const std::string& name, QuestType type,
	const std::string& giver, const std::string& turn_in, const std::string& target,
	int count, const QuestReward& reward, const std::string& text_start, const std::string& text_done)
	: Definition(id, name, text_start), _type(type), _giver(giver), _turn_in(turn_in), _target(target),
	_count(count), _reward(reward), _text_start(text_start), _text_done(text_done) {}

//########################## GETTERS ###########################

QuestType QuestDef::type() const {
	return _type;
}

const std::string& QuestDef::giver() const {
	return _giver;
}

const std::string& QuestDef::turnIn() const {
	return _turn_in;
}

const std::string& QuestDef::target() const {
	return _target;
}

int QuestDef::count() const {
	return _count;
}

const QuestReward& QuestDef::reward() const {
	return _reward;
}

const std::string& QuestDef::textStart() const {
	return _text_start;
}

const std::string& QuestDef::textDone() const {
	return _text_done;
}

//##############################################################################
//#                               QUEST PROGRESS                               #
//##############################################################################

//######################## CONSTRUCTOR #########################

QuestProgress::QuestProgress(const QuestDef& def) : _definition(&def) {}

//########################## GETTERS ###########################

const QuestDef& QuestProgress::definition() const {
	return *_definition;
}

QuestState QuestProgress::state() const {
	return _state;
}

int QuestProgress::kills() const {
	return _kills;
}

//########################## FUNCTIONS #########################

void QuestProgress::addKill() {
	if (_kills < _definition->count())
		++_kills;
}

void QuestProgress::complete() {
	_state = QuestState::COMPLETED;
}
