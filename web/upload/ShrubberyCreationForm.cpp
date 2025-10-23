#include "ShrubberyCreationForm.hpp"

ShrubberyCreationForm::ShrubberyCreationForm() : AForm("Shrubbery Creation Form", 145, 137), _target("default") {
	std::cout << "ShrubberyCreationForm default constructor called" << std::endl;
}

ShrubberyCreationForm::ShrubberyCreationForm(const std::string& target) 
	: AForm("Shrubbery Creation Form", 145, 137), _target(target) {
	std::cout << "ShrubberyCreationForm parametric constructor called" << std::endl;
}

ShrubberyCreationForm::ShrubberyCreationForm(const ShrubberyCreationForm& other) 
	: AForm(other), _target(other._target) {
	std::cout << "ShrubberyCreationForm copy constructor called" << std::endl;
}

ShrubberyCreationForm& ShrubberyCreationForm::operator=(const ShrubberyCreationForm& other) {
	std::cout << "ShrubberyCreationForm copy assignment operator called" << std::endl;
	if (this != &other) {
		AForm::operator=(other);
		_target = other._target;
	}
	return *this;
}

ShrubberyCreationForm::~ShrubberyCreationForm() {
	std::cout << "ShrubberyCreationForm destructor called" << std::endl;
}

const std::string& ShrubberyCreationForm::getTarget() const {
	return _target;
}

void ShrubberyCreationForm::executeAction() const {
	std::string filename = _target + "_shrubbery";
	std::ofstream file(filename);
	
	if (!file.is_open()) {
		throw FileCreationException();
	}
	
	file << "       _-_" << std::endl;
	file << "    /~~   ~~\\" << std::endl;
	file << " /~~         ~~\\" << std::endl;
	file << "{               }" << std::endl;
	file << " \\  _-     -_  /" << std::endl;
	file << "   ~  \\\\ //  ~" << std::endl;
	file << "_- -   | | _- _" << std::endl;
	file << "  _ -  | |   -_" << std::endl;
	file << "      // \\\\" << std::endl;
	file << std::endl;
	
	file << "         ,@@@@@@@," << std::endl;
	file << "       ,,,.   ,,,  @@@@@@/@@," << std::endl;
	file << "    @@@@@@@@  @@@@@@@    @@  @@@@@@@" << std::endl;
	file << "  @@@@@@    @@@@@@@@  @@@@  @@@@@@@@@@@" << std::endl;
	file << " @@@@@@@@@@@@@@@@@@@  @@@@ @@@@@@@@@@@@@" << std::endl;
	file << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
	file << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
	file << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
	file << " @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
	file << "  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
	file << "   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
	file << "      @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
	file << "         @@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
	file << "               |               |" << std::endl;
	file << "               |               |" << std::endl;
	file << "               |_______________|" << std::endl;
	
	file.close();
	std::cout << "Shrubbery has been created in " << filename << std::endl;
}

const char* ShrubberyCreationForm::FileCreationException::what() const throw() {
	return "Failed to create shrubbery file!";
}
