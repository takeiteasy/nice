#!/usr/bin/env lua

-- Convert files to embeddable C strings
-- Usage: lua embed.lua <filename> [variable_name]

local function escape_string(str)
    local escaped = str:gsub("\\", "\\\\")
                      :gsub("\"", "\\\"")
                      :gsub("\n", "\\n")
                      :gsub("\r", "\\r")
                      :gsub("\t", "\\t")
                      :gsub("\0", "\\0")
    return escaped
end

local function get_variable_name(filename)
    local basename = filename:match("([^/\\]+)$") or filename
    local name = basename:gsub("[^%w_]", "_")
    -- Ensure it starts with a letter or underscore
    if name:match("^%d") then
        name = "_" .. name
    end
    return name
end

local function embed_file(filename)
    local file, err = io.open(filename, "rb")
    if not file then
        io.stderr:write("Error: Cannot open file '" .. filename .. "': " .. (err or "unknown error") .. "\n")
        return false
    end
    
    local content = file:read("*a")
    file:close()
    
    local var_name = get_variable_name(filename)
    
    -- Output C string declaration
    print("// Embedded file: " .. filename)
    print("// Size: " .. #content .. " bytes")
    print("static const char " .. var_name .. "[] = ")
    
    -- Split content into lines for better readability
    local lines = {}
    local line_length = 80
    local pos = 1
    
    while pos <= #content do
        local line_end = math.min(pos + line_length - 1, #content)
        local line = content:sub(pos, line_end)
        table.insert(lines, escape_string(line))
        pos = line_end + 1
    end
    
    -- Output each line
    for i, line in ipairs(lines) do
        if i == #lines then
            print("    \"" .. line .. "\";")
        else
            print("    \"" .. line .. "\"")
        end
    end
    
    -- Output size constant
    print("static const size_t " .. var_name .. "_size = " .. #content .. ";")
    print("")
    
    return true
end

-- Main execution
if #arg == 0 then
    io.stderr:write("Usage: lua embed.lua <file1> [file2] [file3] ...\n")
    io.stderr:write("Converts files to embeddable C string constants\n")
    io.stderr:write("Variable names are automatically generated from filenames\n")
    os.exit(1)
end

local failed_files = {}

-- Process all files
for i = 1, #arg do
    local filename = arg[i]
    if not embed_file(filename) then
        table.insert(failed_files, filename)
    end
end

-- Report any failures
if #failed_files > 0 then
    io.stderr:write("Failed to process " .. #failed_files .. " file(s):\n")
    for _, filename in ipairs(failed_files) do
        io.stderr:write("  " .. filename .. "\n")
    end
    os.exit(1)
end