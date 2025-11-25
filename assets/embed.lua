#!/usr/bin/env lua
-- Embed files to C byte arrays, print to stdout
-- Run with ./embed.lua <file1> [file2] [file3] ...

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
    
    -- Output C byte array declaration
    print("\n// Embedded file: " .. filename)
    print("// Size: " .. #content .. " bytes")
    print("static const unsigned char " .. var_name .. "[] = {")
    
    -- Convert content to hex bytes, 16 per line for readability
    local bytes_per_line = 16
    local hex_lines = {}
    
    for i = 1, #content do
        local byte = string.byte(content, i)
        local hex = string.format("0x%02x", byte)
        
        local line_index = math.ceil(i / bytes_per_line)
        if not hex_lines[line_index] then
            hex_lines[line_index] = {}
        end
        table.insert(hex_lines[line_index], hex)
    end
    
    -- Output each line of hex values
    for i, line in ipairs(hex_lines) do
        local line_str = "    " .. table.concat(line, ", ")
        if i == #hex_lines then
            print(line_str)
        else
            print(line_str .. ",")
        end
    end
    
    print("};")
    
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
print("#ifndef NICE_DAT_H")
print("#define NICE_DAT_H")
for i = 1, #arg do
    local filename = arg[i]
    if not embed_file(filename) then
        table.insert(failed_files, filename)
    end
end
print("#endif // NICE_DAT_H")

-- Report any failures
if #failed_files > 0 then
    io.stderr:write("Failed to process " .. #failed_files .. " file(s):\n")
    for _, filename in ipairs(failed_files) do
        io.stderr:write("  " .. filename .. "\n")
    end
    os.exit(1)
end