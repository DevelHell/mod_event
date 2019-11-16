`mod_event` is module for apache2 web server which triggers external command. 
It allows to trigger specific events based on HTTP method and URL, e.g. email alert, detect attacs etc.

# Development

To build and enable the module run:

`apxs -i -a -c mod_event.c`

For more information about apache2 modules development see https://httpd.apache.org/docs/2.4/developer/modguide.html

## macOS CLion note

If you are using CLion under macOS, add following lines to `CMakeLists.txt` for code completion:

```
include_directories (/usr/local/Cellar/apr/1.7.0/libexec/include/apr-1)
include_directories (/usr/local/Cellar/apr-util/1.6.1_1/libexec/include/apr-1)
```

# Usage

Enabling module:

```
EventEnabled On
```

Specifying executable:

```
EventExecutable /var/www/notify.sh
```

If executables requires more arguments, enquote whole command:

```
EventExecutable "/var/www/notify.sh run" 
```

`mod_event` appends three more arguments after executable - method, hostname and unparsed URL. Use `mod_event` inside
`Location` directive to run the executable only when needed.

## Example
```xml
<Location>
    EventEnabled on
    EventExecutable /var/www/notify.sh
</Location>
```

Please note that the executable must be readable and executable by apache user (usually `www-data`).