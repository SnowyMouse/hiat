// For documentation, refer to hiatc's documentation.

#ifndef HAMSTER_IN_A_TUBE_HPP
#define HAMSTER_IN_A_TUBE_HPP

#include "hiat.h"

#include <cstdio>
#include <string>
#include <exception>
#include <memory>
#include <vector>
#include <optional>

namespace HIAT {
    class CompileError : public std::exception {
    public:
        CompileError(const HIATCompileErrorC &error, const char *type) noexcept {
            this->reason = error.message;
            this->file = error.file;
            this->line = error.line;
            this->column = error.column;

            char what_error_c[512];
            std::snprintf(what_error_c, sizeof(what_error_c), "%s:%zu:%zu: %s: %s", this->file.c_str(), this->line, this->column, type, this->reason.c_str());
            what_error = what_error_c;
        };
        const char *what() const noexcept override {
            return this->what_error.c_str();
        }
        std::size_t get_line() const noexcept {
            return this->line;
        }
        std::size_t get_column() const noexcept {
            return this->column;
        }
        const char *get_file() const noexcept {
            return this->file.c_str();
        }
        const char *get_reason() const noexcept {
            return this->reason.c_str();
        }
        ~CompileError() noexcept override {}
    private:
        std::string what_error;
        std::size_t line;
        std::size_t column;
        std::string file;
        std::string reason;
    };

    class Compiler {
    public:
        /**
         * Load the given script
         * 
         * @param script_source_data   pointer to the script source data
         * @param script_source_length length of the script source data
         * @param file_name            name of the file (for error reporting)
         * 
         * @throws HIAT::CompileError on failure
         */
        void read_script_data(const std::uint8_t *script_source_data, std::size_t script_source_length, const char *file_name) {
            HIATCompileErrorC error;
            if(::hiat_compiler_read_script_data(this->get_instance(), file_name, script_source_data, script_source_length, &error) != 0) {
                auto exception = CompileError(error, "error");
                ::hiat_error_free(&error);
                throw exception;
            }
        }

        /**
         * Compile the given script and, if successful, clear all loaded scripts.
         * 
         * @throws HIAT::CompileError on failure
         */
        void compile_scripts() {
            HIATCompileErrorC error;
            auto new_compiled_data = std::unique_ptr<HIATCompiledScriptData, void(*)(HIATCompiledScriptData*)>(::hiat_compiler_compile_script_data(this->get_instance(), &error), ::hiat_script_data_free);
            if(new_compiled_data.get() == nullptr) {
                auto exception = CompileError(error, "error");
                ::hiat_error_free(&error);
                throw exception;
            }
            this->script_data = std::move(new_compiled_data);
        }

        /**
         * Get all scripts compiled from the last call to compile_scripts.
         */
        std::vector<HIATScriptC> get_scripts() {
            std::vector<HIATScriptC> r;
            if(this->script_data.has_value()) {
                auto *script_data = (*this->script_data).get();
                auto script_count = ::hiat_script_data_get_scripts(script_data, nullptr);
                r.resize(script_count);
                ::hiat_script_data_get_scripts(script_data, r.data());
            }
            return r;
        }

        /**
         * Get all globals compiled from the last call to compile_scripts.
         */
        std::vector<HIATGlobalC> get_globals() {
            std::vector<HIATGlobalC> r;
            if(this->script_data.has_value()) {
                auto *script_data = (*this->script_data).get();
                auto global_count = ::hiat_script_data_get_globals(script_data, nullptr);
                r.resize(global_count);
                ::hiat_script_data_get_globals(script_data, r.data());
            }
            return r;
        }

        /**
         * Get all nodes compiled from the last call to compile_scripts.
         */
        std::vector<HIATScriptNodeC> get_nodes() {
            std::vector<HIATScriptNodeC> r;
            if(this->script_data.has_value()) {
                auto *script_data = (*this->script_data).get();
                auto node_count = ::hiat_script_data_get_nodes(script_data, nullptr);
                r.resize(node_count);
                ::hiat_script_data_get_nodes(script_data, r.data());
            }
            return r;
        }

        /**
         * Get all warnings from the last call to compile_scripts.
         */
        std::vector<CompileError> get_warnings() {
            std::vector<CompileError> r;
            if(this->script_data.has_value()) {
                auto *script_data = (*this->script_data).get();
                auto warning_count = ::hiat_script_data_get_warnings(script_data, nullptr);
                r.reserve(warning_count);

                std::vector<HIATCompileErrorC> errors;
                ::hiat_script_data_get_warnings(script_data, errors.data());

                for(auto &e : errors) {
                    r.emplace_back(e, "warning");
                }
            }
            return r;
        }

        /**
         * Get the instance handle
         * 
         * @return instance
         */
        HIATCompiler *get_instance() noexcept {
            return this->instance.get();
        }

        /**
         * Instantiate a compiler instance
         * 
         * @param target   target engine
         * @param encoding target encoding (by default use Windows-1252)
         */
        Compiler(HIATCompileTarget target, HIATCompileEncoding encoding = HIATCompileEncoding::HIAT_Windows1252) : instance(::hiat_compiler_new(target, encoding), ::hiat_compiler_free) {
            if(this->instance.get() == nullptr) {
                throw std::exception();
            }
        }
    private:
        std::unique_ptr<HIATCompiler, void(*)(HIATCompiler*)> instance;
        std::optional<std::unique_ptr<HIATCompiledScriptData, void(*)(HIATCompiledScriptData*)>> script_data;
    };
}

#endif
