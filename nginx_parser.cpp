#include <vector>
#include <string>
#include <iostream>
#include <assert.h>

struct ParsingError : std::exception {
	const char *reason;
	ParsingError(const char *reason) : reason(reason) {}
	const char *what() const throw() {
		return reason;
	}
};

namespace token {
	struct Token {
		virtual ~Token() {};
		virtual void print() = 0;
	};

	struct Semicolon : Token {
		void print() {
			std::cout << ";";
		}
	};

	struct OpeningBrace : Token {
		void print() {
			std::cout << "{";
		}
	};

	struct ClosingBrace : Token {
		void print() {
			std::cout << "}";
		}
	};

	struct String : Token {
		std::string content;
		String(std::string content) : content(content) {}
		void print() {
			std::cout << "'" << content << "'";
		}
	};

	struct Group : Token {
		std::vector<Token *> content;
		Group(std::vector<Token *> content) : content(content) {}
		void print() {
			std::cout << "(";
			for (std::vector<token::Token *>::iterator it = content.begin(); it != content.end(); ++it) {
				(*it)->print();
				std::cout << " ";
			}
			std::cout << ")";
		}
	};

	std::string::iterator skipComment(std::string::iterator input, std::string::iterator end) {
		for (; input != end; ++input) {
			if (*input == '\n') {
				break;
			}
		}
		return input;
	}

	std::string::iterator skipWhitespace(std::string::iterator input, std::string::iterator end) {
		for (; input != end; ++input) {
			if (*input == ' ' || *input == '\t' || *input == '\n')
				continue;
			else if (*input == '#') {
				input++;
				input = skipComment(input, end);
				if (input == end) return input;
				continue;
			}
			else break;
		}

		return input;
	}

	Token *parseWord(std::string::iterator &input, std::string::iterator end) {
		if (input == end) return 0;
		if (*input == ';') {
			input++;
			return new Semicolon;
		}
		if (*input == '{') {
			input++;
			return new OpeningBrace;
		}
		if (*input == '}') {
			input++;
			return new ClosingBrace;
		}
		std::string string;
		for (;;) {
			if (input == end) return new String(string);
			switch (*input) {
			case ' ':
			case '\t':
			case '\n':
			case '#':
			case ';':
			case '{':
			case '}':
				return new String(string);
			default:
			  string.push_back(*input);
			  input++;
			}
		}
	}

	std::vector<Token *> parse(std::string::iterator input, std::string::iterator end) {
		std::vector<Token *> output;

		for (; input != end;) {
			input = skipWhitespace(input, end);
			Token *word = parseWord(input, end);
			if (word) output.push_back(word);
		}

		return output;
	}

	struct ParseGroups {
		std::vector<Token *> content;
		std::vector<Token *>::iterator rest;
	};

	ParseGroups parseGroups(std::vector<Token *>::iterator input, std::vector<Token *>::iterator end) {
		std::vector<Token *> output;

		for (; input != end; ++input) {
			Token *token = *input;

			if (dynamic_cast<ClosingBrace *>(token)) {
				break;
			}
			else if (dynamic_cast<OpeningBrace *>(token)) {
				ParseGroups p = parseGroups(input + 1, end);
				input = p.rest;
				output.push_back(new Group(p.content));
				if (input == end) throw ParsingError("No matching closing curly brace");
			} else {
				output.push_back(token);
			}
		}

		return (ParseGroups){ .content = output, .rest = input };
	}
}

typedef std::vector<token::Token *> Tokens;

using namespace token;

/*
server a b c {
	listen 13;
}

server {
	listen 12 {
		a {
			b {
				lfjadsf aldsfj;
			}
		}
	}

	location / {
		index index.html;
		allowed_methods GET POST;
	}
}

a b c d {
	
}

apples {
	
}

server {
	aslflafdjfsl;; {}
	listen / {
		
	}
	lafsjflj;
	g g ;;

	listen 80;

	listen 80 84024 {}
	listen {
		
	}
}
*/

namespace config {
	struct Directive;
	
	typedef std::vector<Directive> Block;

	void print_block(std::vector<Directive> *, int);
	
	struct Directive {
		std::string name;
		std::vector<std::string> args;
		Block *config;

		void print(int depth) {
			std::cout << name << " ";
			for (std::vector<std::string>::iterator it = args.begin(); it != args.end(); ++it) {
				std::cout << *it << " ";
			}
			if (config) {
				std::cout << "{" << std::endl;
				print_block(config, depth);
				for (int i = 0; i < depth - 1; i++) std::cout << "\t";
				std::cout << "}";
			} else {
				std::cout << "; ";
			}
		}
	};

	void print_block(Block *b, int depth) {
		for (Block::iterator it = b->begin(); it != b->end(); ++it) {
			for (int i = 0; i < depth; i++) std::cout << "\t";
			it->print(depth + 1);
			std::cout << std::endl;
		}
	}

	Block *parse_block(std::vector<Token *>::iterator input, std::vector<Token *>::iterator end) {
		Block *output = new Block;

		for (;;) {
			if (input == end) return output;
			Directive d;
			d.name = dynamic_cast<String &>(**input).content;
			d.config = 0;
			input++;
			// Parse args.
			for (;;) {
				if (input == end) throw ParsingError("parse_block: directive: unexpected end of input");
				if (String *s = dynamic_cast<String *>(*input)) {
					d.args.push_back(s->content);
					input++;
				} else break;
			}
			// Should be either ; or {.
			if (input == end) throw ParsingError("unexpected end of input");
			if (dynamic_cast<Semicolon *>(*input)) {
				input++;
				output->push_back(d);
				continue;
			}
			else if (Group *g = dynamic_cast<Group *>(*input)) {
				Block *b = parse_block(g->content.begin(), g->content.end());
				input++;
				d.config = b;
				output->push_back(d);
			}
			else throw ParsingError("parse_block: directive: expected ; or {");
		}

		return output;
	}
}

void print_tokens(Tokens tokens) {
	Group g(tokens);
	g.print();
}

int main() {
	Tokens tokens;
	tokens.push_back(new token::OpeningBrace);
	tokens.push_back(new token::OpeningBrace);
	tokens.push_back(new token::String("hello"));
	tokens.push_back(new token::ClosingBrace);
	tokens.push_back(new token::ClosingBrace);
	tokens.push_back(new token::OpeningBrace);
	tokens.push_back(new token::String("hello 2"));
	tokens.push_back(new token::ClosingBrace);

	print_tokens(tokens);
	std::cout << std::endl;

	token::ParseGroups p = parseGroups(tokens.begin(), tokens.end());

	print_tokens(p.content);
	std::cout << std::endl;

	{
		std::string string =
			"# Comment.\n"
			"   hello ladlfasdf\n"
		  "goodbye"
		;
		std::string::iterator hello = skipWhitespace(string.begin(), string.end());
		hello = skipWhitespace(hello, string.end());
		std::string result(hello, string.end());
		std::cout << result << std::endl;
	}

	{
		std::string string = " ";
		std::string::iterator begin = string.begin();
		Token *t = parseWord(begin, string.end());
		if (t) {
			t->print();
			std::cout << std::endl;
		}
	}

	{
		std::string input =
		  "server { location / { a; b d; } }"
		  // "server { # shit"
		  // "a; b; c;;"
		;
		Tokens tokens = parse(input.begin(), input.end());
		print_tokens(tokens);
		std::cout << std::endl;
		ParseGroups g = parseGroups(tokens.begin(), tokens.end());
		std::cout << "g content" << std::endl;
		print_tokens(g.content);
		std::cout << std::endl;

		config::Block *b = config::parse_block(g.content.begin(), g.content.end());
		std::cout << "Printing a block" << std::endl;
		config::print_block(b, 0);
	}
}
