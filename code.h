#ifndef DDSN_CODE_H
#define DDSN_CODE_H

#include <string>

namespace ddsn {
	class Code {
	public:
		Code();
		Code(int layers);
		Code(int layers, const char *code);
		Code(std::string code);
		Code(const Code &code);

		~Code();

		int get_layer_code(int layer) const;
		int get_ext_layer_code(int layer) const;
		void set_layer_code(int layer, int code);

		void resize_layers(int layers);

		int get_layers() const;

		bool contains(const Code &code) const;
		int get_differing_layer(const Code &code) const;

		Code &operator=(const Code &region);

		std::string get_string() const;

		friend std::ostream& operator<<(std::ostream& stream, const Code& code);
	private:
		int layers_;
		char *code_;
	};

	std::ostream& operator<<(std::ostream& stream, const Code& code);
}

#endif