# Currently an error run via --script cancels the script, but the gui seems to
# capture the error messages so this script doesn't receive the error.

# We can catch failures by wrapping the whole test in a try, but we should find
# a better method or at least write a wrapper script that automatically wraps
# our tests in a try catch.
try {
    # Try finding garbage
    qt_findWidgets -type fakeType
    # for this case we want to make sure that an error is caught so if we
    # make it this far, return an error
    exit 1
} on error {msg} {
    puts "Successfully caught error"
    exit 0
}
