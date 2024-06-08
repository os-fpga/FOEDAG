package require tcltest  ; # Make sure tcltest is loaded first
namespace import -force ::tcltest::*

# Test setup: Define device and map names
device_name test

# Mapping RTL user names to RTL names
map_rtl_user_names -user_name us_b -rtl_name rt_b

# Mapping model user names to model names
map_model_user_names -model_name mo_a -user_name us_a

# Test cases for RTL user name mapping
test map_rtl_user_name {Map RTL user name to RTL name} {
    get_rtl_name -user_name us_b
} {rt_b}

# Test cases for model user name mapping
test map_model_user_name {Map model user name to model name} {
    get_model_name -user_name us_a
} {mo_a}

test get_user_name_from_model {Get user name from model name} {
    get_user_name -model_name mo_a
} {us_a}

# Test cases for error handling
test missing_rtl_user_name {Missing RTL user name} {
    catch {get_rtl_name} err
    set err
} {Insufficient arguments passed to get_rtl_name.}

test invalid_rtl_user_name {Invalid RTL user name} {
    catch {get_rtl_name -user_name non_existent_user} err
    set err
} {}

test missing_model_user_name {Missing model user name} {
    catch {get_model_name} err
    set err
} {Insufficient arguments passed to get_model_name.}

test invalid_model_user_name {Invalid model user name} {
    catch {get_model_name -user_name non_existent_user} err
    set err
} {User name non_existent_user does not exist in model mapping.}

test missing_model_name {Missing model name} {
    catch {get_user_name} err
    set err
} {Insufficient arguments passed to get_user_name.}

test invalid_model_name {Invalid model name} {
    catch {get_user_name -model_name non_existent_model} err
    set err
} {}

cleanupTests
