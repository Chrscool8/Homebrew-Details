project('Homebrew Details', ['c', 'cpp'],
    version: '1.0.0',
    default_options: [ 'buildtype=release', 'strip=true', 'b_ndebug=if-release', 'cpp_std=c++1z' ],
)

subdir('library')

program_files = files(
    'src/main.cpp',
    'src/nacp_win.cpp',
    'src/pages/apps_list_page.cpp',
    'src/pages/info_page.cpp',
    'src/pages/intro_page.cpp',
    'src/pages/issue_page.cpp',
    'src/pages/layout_select_page.cpp',
    'src/pages/main_page.cpp',
    'src/pages/scanning_page.cpp',
    'src/pages/updating_page.cpp',
    'src/utils/base64.cpp',
    'src/utils/blacklist.cpp',
    'src/utils/favorites.cpp',
    'src/utils/launching.cpp',
    'src/utils/modules.cpp',
    'src/utils/nacp_utils.cpp',
    'src/utils/notes.cpp',
    'src/utils/panels.cpp',
    'src/utils/reboot_to_payload.cpp',
    'src/utils/scanning.cpp',
    'src/utils/settings.cpp',
    'src/utils/update.cpp',
    'src/utils/utilities.cpp'
)

borealis_example = executable(
    'Homebrew Details',
    [ program_files, borealis_files ],
    dependencies : borealis_dependencies,
    install: true,
    include_directories: 
	[ 
	borealis_include, 
	include_directories('src'),
	include_directories('src/pages'),
	include_directories('src/utils'),
	include_directories('include'),
	include_directories('include/extern')
	],
    cpp_args: [ '-g', '-O2', '-DBOREALIS_RESOURCES="./resources/"', '-DAPP_VERSION="1"' ]
)
