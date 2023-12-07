project(
    'sound',
    'c',
    version : '1.0.0',
    meson_version : '>= 0.54.0'
)

# Setup variables
version_arr = meson.project_version().split('.')
prj_name = meson.project_name()
serviceName = 'defigo-' + prj_name
baseName = 'com.getdefigo.'
basePath = '/com/getdefigo/'
thisName = baseName + prj_name
prefix = get_option('prefix')
serviceConfigPath = prefix / get_option('libdir') / 'systemd' / 'system'
servicePolicyPath = prefix / get_option('sysconfdir') / 'dbus-1' / 'system.d'
execPath = prefix / get_option('bindir')
execFile = execPath / prj_name
execParams = ''

# Read user params
sysUser = get_option('user')
logLevel = get_option('log_level')

# Configure service command line
if logLevel == 'err'
    execParams = ' -q'
elif logLevel == 'info'
    execParams = ' -v'
elif logLevel == 'notice'
    execParams = ' -vv'
elif logLevel == 'dbg'
    execParams = ' -vvv'
endif


# Project configuration
conf_data = configuration_data()
conf_data.set('version_major',      version_arr[0])
conf_data.set('version_minor',      version_arr[1])
conf_data.set('version_rev',        version_arr[2])
conf_data.set('this_name',          thisName)
conf_data.set('dbus_base_path',     basePath)
conf_data.set('dbus_base_name',     baseName)
conf_data.set('prj_name',           prj_name)
conf_data.set('user',               sysUser)
conf_data.set('executable',         execFile + execParams)
conf_data.set('log_file_path',      '/var/log/defigo-' + prj_name + '.log')


# Dependencies
deps = [
    dependency('libsystemd'),
    dependency('threads'),
    dependency('alsa'),
    dependency('libcurl'),
    dependency('yaml-0.1')
]


# Includes
inc = include_directories([
    'include',
    'libs/libcyaml/include',
])

# Sources
src = [
    'libs/libcyaml/src/copy.c',
    'libs/libcyaml/src/free.c',
    'libs/libcyaml/src/load.c',
    'libs/libcyaml/src/mem.c',
    'libs/libcyaml/src/save.c',
    'libs/libcyaml/src/utf8.c',
    'libs/libcyaml/src/util.c',

    'src/app.c',
    'src/bus.c',
    'src/sound.c',
    'src/mixer.c',
    'src/config.c',
    'src/download.c'
]

# Main header file config
configure_file(
    input         : 'src/app.h.in',
    output        : 'app.h',
    configuration : conf_data
)

# Service config file
configure_file(
    input         : 'files/config.service',
    output        : serviceName + '.service',
    configuration : conf_data,
    install       : true,
    install_mode  : 'rw-r--r--',
    install_dir   : serviceConfigPath
)

# Service policy file
configure_file(
    input         : 'files/policy.xml',
    output        : thisName + '.conf',
    configuration : conf_data,
    install       : true,
    install_mode  : 'rw-r--r--',
    install_dir   : servicePolicyPath
)

# Build executable
executable(
    prj_name,
    src,
    include_directories : inc,
    dependencies        : deps,
    install             : true,
    install_dir         : execPath
)