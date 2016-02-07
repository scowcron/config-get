#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <libconfig.h>
#include <assert.h>
#include <stdbool.h>

enum value_type {
	vt_unspec,
	vt_number,
	vt_string,
	vt_bool,
};

enum match_type {
	mt_strict,
	mt_loose,
	mt_warn,
	mt_ignore,
};

static void print_usage(const char *progname, FILE *dest) {
	fprintf(dest,
			"Usage: %s [opts] key\n"
			"\n"
			"Options:\n"
			"  -h, --help         show this help text and exit\n"
			"  -f, --file=FILE    path to configuration file\n"
			"  -t, --type=TYPE    expected type of value\n"
			"  -m, --match=MATCH  type matching behavior\n"
			"\n"
			"For more information, see `man 1 config-get`\n",
			progname);
}

int parse_match_type(const char *str, enum match_type *out) {
	if (strcmp("strict", str) == 0) {
		*out = mt_strict;
	} else if(strcmp("loose", str) == 0) {
		*out = mt_loose;
	} else if(strcmp("warn", str) == 0) {
		*out = mt_warn;
	} else if(strcmp("ignore", str) == 0) {
		*out = mt_ignore;
	} else {
		return 1;
	}
	return 0;
}

int parse_value_type(const char *str, enum value_type *out) {
	if (strcmp("number", str) == 0) {
		*out = vt_number;
	} else if(strcmp("string", str) == 0) {
		*out = vt_string;
	} else if(strcmp("boolean", str) == 0) {
		*out = vt_bool;
	} else {
		return 1;
	}
	return 0;

}

const char *value_type_string(enum value_type type) {
	switch (type) {
		case vt_unspec:
			return "unspecified";
		case vt_number:
			return "number";
		case vt_string:
			return "string";
		case vt_bool:
			return "boolean";
		default:
			assert(false && "unknown value type");
	}
}

config_t *read_config(const char *fname) {
	config_t *cfg = malloc(sizeof(config_t));
	config_init(cfg);

	int rc;
	if (fname != NULL) {
		rc = config_read_file(cfg, fname);
	} else {
		rc = config_read(cfg, stdin);
	}

	if (rc == CONFIG_FALSE) {
		fprintf(stderr, "Error reading config: %s (line %d)\n",
				config_error_text(cfg),
				config_error_line(cfg));
		config_destroy(cfg);
		free(cfg);
		return NULL;
	}

	return cfg;
}

void print_setting(config_setting_t *setting) {
	assert(setting != NULL);
	assert(config_setting_is_scalar(setting));

	switch (config_setting_type(setting)) {
		case CONFIG_TYPE_INT:
			printf("%d\n", config_setting_get_int(setting));
			break;
		case CONFIG_TYPE_INT64:
			printf("%lld\n", config_setting_get_int64(setting));
			break;
		case CONFIG_TYPE_FLOAT:
			printf("%g\n", config_setting_get_float(setting));
			break;
		case CONFIG_TYPE_STRING:
			printf("%s\n", config_setting_get_string(setting));
			break;
		case CONFIG_TYPE_BOOL:
			printf("%s\n", config_setting_get_bool(setting) ? "true" : "false");
			break;
	}
}

int print_key(config_t *cfg, const char *key, enum value_type expect_type, enum match_type match) {
	int exit_code = EXIT_SUCCESS;

	config_setting_t *setting = config_lookup(cfg, key);
	if (setting == NULL) {
		fprintf(stderr, "Key (%s) not in config.\n", key);
		return EXIT_FAILURE;
	}

	enum value_type actual_type;
	switch (config_setting_type(setting)) {
		case CONFIG_TYPE_INT:
		case CONFIG_TYPE_INT64:
		case CONFIG_TYPE_FLOAT:
			actual_type = vt_number;
			break;
		case CONFIG_TYPE_STRING:
			actual_type = vt_string;
			break;
		case CONFIG_TYPE_BOOL:
			actual_type = vt_bool;
			break;
		default:
			fprintf(stderr, "Found aggregate setting.\n");
			return EXIT_FAILURE;
	}

	// check type match
	if (actual_type != expect_type && expect_type != vt_unspec) {
		switch (match) {
			case mt_strict:
				return EXIT_FAILURE;
			case mt_loose:
				exit_code = EXIT_FAILURE;
				break;
			case mt_warn:
				fprintf(stderr, "Expected type (%s) does not match found (%s).\n",
						value_type_string(expect_type),
						value_type_string(actual_type));
				break;
			case mt_ignore:
				break;
		}
	}

	print_setting(setting);
	return exit_code;

}

int main(int argc, char *argv[]) {
	const char *progname = argv[0];

	const char *key = NULL;
	const char *filename = NULL;
	enum value_type expect_type = vt_unspec;
	enum match_type match = mt_warn;

	// Read command line opts
	{
		for (;;) {
			static struct option long_opts[] = {
				{ "file", required_argument, 0, 'f' },
				{ "help", no_argument, 0, 'h' },
				{ "match", required_argument, 0, 'm' },
				{ "type", required_argument, 0, 't' },
				{ 0, 0, 0, 0},
			};

			int rc;
			int opt_index = 0;
			int c = getopt_long(argc, argv, "f:hm:t:", long_opts, &opt_index);

			if (c == -1) {
				// end of arguments
				break;
			}

			switch (c) {
				case 'f':
					filename = optarg;
					break;
				case 'h':
					print_usage(progname, stdout);
					return EXIT_SUCCESS;
				case 'm':
					rc = parse_match_type(optarg, &match);
					if (rc != 0) {
						fprintf(stderr, "Unrecognized match type: %s\n", optarg);
						return EXIT_FAILURE;
					}
					break;
				case 't':
					rc = parse_value_type(optarg, &expect_type);
					if (rc != 0) {
						fprintf(stderr, "Unrecognized value type: %s\n", optarg);
						return EXIT_FAILURE;
					}
					break;
			}
		}

		// No key was specified
		if (optind == argc) {
			print_usage(progname, stderr);
			return EXIT_FAILURE;
		}

		key = argv[optind++];

		// too many positional arguments
		if (optind != argc) {
			fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
			print_usage(progname, stderr);
			return EXIT_FAILURE;
		}
	}

	config_t *cfg = read_config(filename);
	if (cfg == NULL) {
		return EXIT_FAILURE;
	}

	int exit_code = print_key(cfg, key, expect_type, match);

	config_destroy(cfg);
	free(cfg);
	return exit_code;

}
