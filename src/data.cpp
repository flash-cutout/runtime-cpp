#include "data.hpp"

#include <msgpack.hpp>
#include <fstream>
#include <string>
#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/filewritestream.h>
#include <msgpack/type/rapidjson/document.hpp>

namespace {


bool read_file_contents(const std::string& filename, std::string* contents)
{
	std::ifstream in(filename, std::ios::in | std::ios::binary);
	if (in.bad())
		return false;

	in.seekg(0, std::ios::end);
	contents->resize(static_cast<size_t>(in.tellg()));
	in.seekg(0, std::ios::beg);
	in.read(&contents->front(), contents->size());
	in.close();
	return true;
}
bool write_file_contents(const std::string& filename, const std::string& contents)
{
	std::ofstream of(filename, std::ios::out | std::ios::binary);
	if (of.bad())
		return false;
	of.write(contents.data(), contents.size());
	return true;
}

} // namespace

namespace cutout {

void Matrix::json_unpack(const rapidjson::Value& o)
{
	using namespace rapidjson;
	if (!o.IsArray()) { throw std::domain_error(""); }


	const size_t size = o.Size();
	if (size > 0) {
		Value::ConstValueIterator ptr = o.Begin();
		switch (size) {
		default:
		case 6: tx = ptr[5].GetDouble();
		case 5: ty = ptr[4].GetDouble();
		case 4: sx = ptr[3].GetDouble();
		case 3: shy = ptr[2].GetDouble();
		case 2: shx = ptr[1].GetDouble();
		case 1: sy = ptr[0].GetDouble();
		}
	}
}






template <typename T>
struct JSON {
	static void convert(const rapidjson::Value &j, T* t)
	{
		t->json_unpack(j);
	}
};
template<>
struct JSON<unsigned>{
	static void convert(const rapidjson::Value &j, unsigned* t)
	{
		*t = j.GetUint();
	}
};
template<typename T>
struct JSON<std::vector<T>>{
	static void convert(const rapidjson::Value &j, std::vector<T>* t)
	{
		using namespace rapidjson;
		if (!j.IsArray())
			throw std::runtime_error("array required");
		Value::ConstValueIterator i = j.Begin(), END = j.End();
		for (; i != END; ++i)
		{
			T v;
			JSON<T>::convert(*i, &v);
			t->push_back(v);
		}
	}
};

template<>
struct JSON<std::string>{
	static void convert(const rapidjson::Value &j, std::string* t)
	{
		t->assign(j.GetString(), j.GetStringLength());
	}
};

/*
template <>
struct JSON<std::map<std::string, Symbol>> {
static void convert(const rapidjson::Value &j, std::map<std::string, Symbol>* t)
{
using namespace rapidjson;

Value::ConstMemberIterator i = j.MemberBegin();
Value::ConstMemberIterator END = j.MemberEnd();
for (; i != END; ++i)
{
std::pair<std::string, Symbol> v;
v.first.assign(i->name.GetString(), i->name.GetStringLength());
v.second.json_unpack(i->value);
t->insert(v);
}
}
};
*/
template <typename T>
struct JSON<std::map<std::string, T>> {
	static void convert(const rapidjson::Value &j, std::map<std::string, T>* t)
	{
		using namespace rapidjson;

		Value::ConstMemberIterator i = j.MemberBegin();
		Value::ConstMemberIterator END = j.MemberEnd();
		for (; i != END; ++i)
		{
			std::pair<std::string, T> v;
			v.first.assign(i->name.GetString(), i->name.GetStringLength());
			JSON<T>::convert(i->value, &v.second);
			t->insert(v);
		}
	}
};


template <typename T>
void json_convert(const rapidjson::Value &j, T* t)
{
	JSON<T>::convert(j, t);
}


Data::Data(const char* filename)
{
	std::string extension(strrchr(filename, '.'));
	std::string buf;
	Format::Type format = Format::cast(extension);
	if (format == Format::INVALID)
		throw std::domain_error("Unsupported data type:" + extension);

	if (!read_file_contents(filename, &buf))
		throw std::runtime_error(std::string("Error while reading :") + filename);
	
	switch (format)
	{
	case Format::JSON: loadJSON(buf); break;
	case Format::MPACK: loadMPACK(buf); break;

	case Format::INVALID:
	default:
		break;
	}
}


void Data::msgpack_unpack(msgpack::object o)
{
	using namespace msgpack;

	if (o.type != type::MAP)
		return throw type_error();

	for (size_t i = 0; i < o.via.map.size; ++i)
	{
		if (o.type != type::MAP) { throw type_error(); }
		object_kv* p(o.via.map.ptr);
		object_kv* const pend(o.via.map.ptr + o.via.map.size);

		std::string kname;
		for (; p != pend; ++p) {
			if (p->val.type != type::MAP)
				throw type_error();

			p->key.convert(&kname);
			switch (Field::cast(kname))
			{
			case Field::SCENES: p->val.convert(&this->scenes); break;
			case Field::ANIMATIONS: p->val.convert(&this->animations); break;
			case Field::PARTS: p->val.convert(&this->parts); break;

			case Field::INVALID:
			default: break;
			}
		}
	}
}


void Data::json_unpack(const rapidjson::Value& o)
{
	using namespace rapidjson;

	if (!o.IsObject())
		return throw std::domain_error("value is not a Object");

	Value::ConstMemberIterator m, END = o.MemberEnd();

	if ((m = o.FindMember("scenes")) != END)
		json_convert(m->value, &scenes);

	if ((m = o.FindMember("animations")) != END)
		json_convert(m->value, &animations);

	if ((m = o.FindMember("parts")) != END)
		json_convert(m->value, &parts);
}

void Symbol::json_unpack(const rapidjson::Value& o)
{
	using namespace rapidjson;
	if (!o.IsObject())
		throw std::domain_error("Object required.");

	Value::ConstMemberIterator m, mend = o.MemberEnd();

	m = o.FindMember("duration");
	if (m != mend)
		duration = m->value.GetUint();

	m = o.FindMember("fps");
	if (m != mend)
		fps = m->value.GetUint();

	m = o.FindMember("labels");
	if (m != mend)
		json_convert(m->value, &labels);

	m = o.FindMember("objects");
	if (m != mend)
		json_convert(m->value, &objects);
}

void Symbol::msgpack_unpack(msgpack::object o)
{
	using namespace msgpack;
	if (o.type != type::MAP)
		throw type_error();
	object_kv* p(o.via.map.ptr);
	object_kv* const pend(o.via.map.ptr + o.via.map.size);

	std::string kname;
	for (; p != pend; ++p) {
		p->key.convert(&kname);
		switch (Field::cast(kname))
		{
		case Field::DURATION: p->val.convert(&duration); break;
		case Field::FPS: p->val.convert(&fps); break;
		case Field::LABELS: p->val.convert(&labels); break;
		case Field::OBJECTS: p->val.convert(&objects); break;

		case Field::INVALID:
		default:break;
		}
	}
}

unsigned Symbol::index(const std::string& name) const
{
	std::map<std::string, unsigned>::const_iterator p = labels.find(name);
	if (p == labels.end())
		return (unsigned)(-1);
	return p->second;
}

const std::string* Symbol::lable(unsigned index) const
{
	std::map<std::string, unsigned>::const_iterator p = labels.begin(),
		END = labels.end();
	for (; p != END; ++p)
	{
		if (p->second == index)
			return &(p->first);
	}
	return NULL;
}



void Keyframe::json_unpack(const rapidjson::Value& o)
{
	using namespace rapidjson;
	if (!o.IsObject())
		throw std::domain_error("..");

	Value::ConstMemberIterator m, mend = o.MemberEnd();

	m = o.FindMember("cmd");
	if (m != mend)
		cmd = Command::cast(std::string(m->value.GetString(), m->value.GetStringLength()));
	m = o.FindMember("duration");
	if (m != mend)
		json_convert(m->value, &duration);

	m = o.FindMember("character");
	if (m != mend)
		json_convert(m->value, &character);

	m = o.FindMember("matrix");
	if (m != mend)
		json_convert(m->value, &matrix);

	m = o.FindMember("name");
	if (m != mend)
		json_convert(m->value, &name);

	m = o.FindMember("tween");
	if (m != mend)
		json_convert(m->value, &tween);
}

void Keyframe::msgpack_unpack(msgpack::object o)
{
	using namespace msgpack;
	if (o.type != type::MAP)
		throw type_error();
	object_kv* p(o.via.map.ptr);
	object_kv* const pend(o.via.map.ptr + o.via.map.size);

	std::string kname;
	for (; p != pend; ++p) {
		p->key.convert(&kname);
		switch (Field::cast(kname))
		{
		case Field::CMD: cmd = Command::cast(p->val.as<std::string>()); break;
		case Field::DURATION: p->val.convert(&duration); break;
		case Field::CHARACTER: p->val.convert(&character); break;
		case Field::MATRIX: p->val.convert(&matrix); break;
		case Field::NAME: p->val.convert(&name); break;
		case Field::TWEEN: p->val.convert(&tween); break;

		case Field::INVALID:
		default:break; // unsupported field.
		}
	}
}




void Tween::json_unpack(const rapidjson::Value& o)
{
	using namespace rapidjson;
	if (!o.IsArray()) { throw std::domain_error(""); }

	const size_t size = o.Size();
	if (size > 0) {
		Value::ConstValueIterator ptr = o.Begin();
		switch (size) {
		default:
		case 3:rotateTimes = ptr[2].GetUint();
		case 2:rotateDirection = Rotation::cast(std::string(ptr[1].GetString(), ptr[1].GetStringLength()));
		case 1:tween = ptr[0].GetBool();
		}
	}
}


void Data::loadMPACK(std::string& buf)
{

	msgpack::unpacked msg;
	msgpack::unpack(&msg, buf.data(), buf.size());

	msg.get().convert(this);
#ifdef _DEBUG // VERIFY
	rapidjson::Document doc;

	msg.get().convert(&doc);
	using namespace rapidjson;

	FILE* fp = fopen("D:/Dragon.mpack.json", "wb");
	char writeBuffer[65536];
	FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
	Writer<FileWriteStream> writer(os);
	doc.Accept(writer);
	fclose(fp);
#endif
}

void Data::loadJSON(std::string& buf)
{
	using namespace rapidjson;
	Document doc;
	doc.Parse(buf.c_str());
	json_convert(doc, this);

	FILE* fp = fopen("D:/Dragon.copy.json", "wb");
	char writeBuffer[65536];
	FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
	Writer<FileWriteStream> writer(os);
	doc.Accept(writer);
	fclose(fp);

	msgpack::sbuffer sbuf;  // simple buffer
	msgpack::pack(&sbuf, doc);
	write_file_contents("D:/Dragon.mpack", std::string(sbuf.data(), sbuf.size()));
}




Keyframe::Keyframe(Command::Type acmd, unsigned aindex, unsigned aduration)
	: cmd(acmd), index(aindex), duration(aduration)
{

}


std::vector<std::string> Keyframe::Command::_vector;


std::map<std::string, Keyframe::Command::Type> Keyframe::Command::_map;


std::map<std::string, Tween::Rotation::Type> Tween::Rotation::_map;


std::vector<std::string> Tween::Rotation::_vector;


void Sprite::json_unpack(const rapidjson::Value& o)
{
	using namespace rapidjson;
	if (!o.IsArray()) { throw std::runtime_error(""); }


	const size_t size = o.Size();
	if (size > 0) {
		Value::ConstValueIterator ptr = o.Begin();
		switch (size) {
		default:
		case 8:  frame.x = ptr[7].GetDouble();
		case 7:  frame.y = ptr[6].GetDouble();
		case 6:  frame.w = ptr[5].GetDouble();
		case 5:  frame.h = ptr[4].GetDouble();
		case 4: sprite.x = ptr[3].GetDouble();
		case 3: sprite.y = ptr[2].GetDouble();
		case 2: sprite.w = ptr[1].GetDouble();
		case 1: sprite.h = ptr[0].GetDouble();
		}
	}
}

} // namespace...


