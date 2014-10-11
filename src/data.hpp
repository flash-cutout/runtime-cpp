#ifndef __LIBCUTOUT_DATA_HPP__
#define __LIBCUTOUT_DATA_HPP__

#include <vector>
#include <map>
#include <string>
#include <msgpack.hpp>
#include <rapidjson/document.h>

namespace cutout {
class Symbol;

class Matrix {
	float sx, shy, tx,
	shx, sy, ty;
public:
	Matrix() :sx(1), shy(0), tx(0), shx(0), sy(1), ty(0) {}
	MSGPACK_DEFINE(tx, ty, sx, shy, shx, sy);
	void json_unpack(const rapidjson::Value& o);
};
struct Rect {
	int x, y;
	unsigned w, h;
};



struct Tween {
	struct Rotation {
		enum Type {
			NONE,
			AUTO,
			CW,
			CCW
		};

		static Type cast(const std::string& name)
		{
			if (_map.empty())
			{
				_map["none"] = NONE;
				_map["auto"] = AUTO;
				_map["cw"] = CW;
				_map["ccw"] = CCW;
			}
			std::map<std::string, Type>::iterator i = _map.find(name);
			return (i != _map.end()) ? i->second : NONE;
		}
		static std::string cast(Type type)
		{
			if (_vector.empty())
			{
				_vector.push_back("none");
				_vector.push_back("auto");
				_vector.push_back("cw");
				_vector.push_back("ccw");
			}
			return (static_cast<unsigned>(type) <= static_cast<unsigned>(CCW)) ? _vector[type] : "none";
		}
	private:
		static std::map<std::string, Type> _map;
		static std::vector<std::string> _vector;
	};
	Tween() : tween(false), rotateDirection(Rotation::NONE), rotateTimes(0) {}

	bool tween; // default to false
	unsigned rotateDirection; // default to AUTO
	unsigned rotateTimes; // default to 0
	void msgpack_unpack(msgpack::object o)
	{
		if (o.type != msgpack::type::ARRAY)
			throw msgpack::type_error();
		msgpack::object* ptr = o.via.array.ptr;

		switch (o.via.array.size)
		{
		case 3:ptr[2].convert(&rotateTimes);
		case 2:rotateDirection = Rotation::cast(ptr[1].as<std::string>());
		case 1:ptr[0].convert(&tween);
		}
	}
	void json_unpack(const rapidjson::Value& o);
};
class Keyframe
{
public:
	// cmd types
	struct Command {
		enum Type {
			INVALID,
			PLACE,
			MOVE,
			REPLACE,
			REMOVE
		};
		static Type cast(const std::string& name)
		{
			if (_map.empty())
			{
				_map["invalid"] = INVALID;
				_map["place"] = PLACE;
				_map["move"] = MOVE;
				_map["replace"] = REPLACE;
				_map["remove"] = REMOVE;
			}
			std::map<std::string, Type>::iterator i = _map.find(name);
			return (i != _map.end()) ? i->second : INVALID;
		}
		static std::string cast(Type type)
		{
			if (_vector.empty())
			{
				_vector.push_back("invalid");
				_vector.push_back("place");
				_vector.push_back("move");
				_vector.push_back("replace");
				_vector.push_back("remove");
			}
			return (static_cast<unsigned>(type) <= static_cast<unsigned>(REMOVE)) ? _vector[type] : "invalid";
		}
	private:
		static std::map<std::string, Type> _map;
		static std::vector<std::string> _vector;
	};

	Keyframe(Command::Type acmd = Command::INVALID, unsigned aindex = 0, unsigned aduration = 0);

	Command::Type cmd;
	unsigned index;
	unsigned duration;
	std::string character; // optional
	Matrix matrix; // optional
	std::string name; // optional
	Tween tween; // optional

	void msgpack_unpack(msgpack::object o);
	void json_unpack(const rapidjson::Value& o);
private:
	struct Field
	{
		enum Name {
			INVALID,
			CMD,
			INDEX,
			DURATION,
			CHARACTER,
			MATRIX,
			NAME,
			TWEEN
		};

		static Name cast(const std::string& name)
		{
			static std::map<std::string, Name> map_;
			if (map_.empty())
			{
				map_["cmd"] = CMD;
				map_["index"] = INDEX;
				map_["duration"] = DURATION;
				map_["character"] = CHARACTER;
				map_["matrix"] = MATRIX;
				map_["name"] = NAME;
				map_["tween"] = TWEEN;
			}
			std::map<std::string, Name>::const_iterator pos = map_.find(name);
			return pos == map_.end() ? INVALID : pos->second;
		}
	};
};



class Sprite{
	Rect frame;
	Rect sprite;
	bool rotated;
	bool trimmed;
public:
	MSGPACK_DEFINE(frame.x, frame.y, frame.w, frame.h, sprite.x, sprite.y, sprite.w, sprite.h);
	void json_unpack(const rapidjson::Value& o);
};


class Symbol {
	unsigned duration;
	unsigned fps;
	std::vector<std::vector<Keyframe>> objects;
	std::map<std::string, unsigned> labels;
private:
	struct Field
	{
		enum Name {
			INVALID,
			DURATION,
			FPS,
			LABELS,
			OBJECTS
		};

		static Name cast(const std::string& name)
		{
			static std::map<std::string, Name> map_;
			if (map_.empty())
			{
				map_["duration"] = DURATION;
				map_["fps"] = FPS;
				map_["labels"] = LABELS;
				map_["objects"] = OBJECTS;
			}
			std::map<std::string, Name>::const_iterator pos = map_.find(name);
			return pos == map_.end() ? INVALID : pos->second;
		}
	};
public:
	friend class Frame;
	unsigned index(const std::string& name) const;
	const std::string* lable(unsigned index) const;
	std::pair<Keyframe*, Keyframe*> getKeyframes(unsigned depth, float time);
	// Data support APIs
	void msgpack_unpack(msgpack::object o);
	void json_unpack(const rapidjson::Value& value);
};

class Data
{
public:
	struct Format {
		enum Type {
			INVALID,
			JSON,
			MPACK
		};
		static Type cast(std::string& extname)
		{
			std::map<std::string, Type> map_;
			if (map_.empty())
			{
				map_[".json"] = JSON;
				map_[".mpack"] = MPACK;
			}
			std::map<std::string, Type>::const_iterator pos = map_.find(extname);
			return pos == map_.end() ? INVALID : pos->second;
		}
	};
	explicit Data(const char* filename);

private:
	void loadMPACK(std::string& buf);
	void loadJSON(std::string& buf);

protected:
private:
	std::map<std::string, Symbol> scenes;
	std::map<std::string, Symbol> animations;
	std::map<std::string, std::vector<Sprite>> parts;

	struct Field
	{
		enum Name {
			INVALID,
			SCENES,
			ANIMATIONS,
			PARTS
		};

		static Name cast(const std::string& name)
		{
			static std::map<std::string, Name> map_;
			if (map_.empty())
			{
				map_["scenes"] = SCENES;
				map_["animations"] = ANIMATIONS;
				map_["parts"] = PARTS;
			}
			std::map<std::string, Name>::const_iterator pos = map_.find(name);
			return pos == map_.end() ? INVALID : pos->second;
		}
	};
public:
	void msgpack_unpack(msgpack::object o);
	void json_unpack(const rapidjson::Value& o);
};

} // namespace cutout

#endif // __LIBCUTOUT_DATA_HPP__
