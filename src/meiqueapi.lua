function abortIf(var, message)
    if var then
       error(message, 0)
    end
end

-- Note: This function return false if the metatable of some object is itself in the metatable chain.
function instanceOf(instance, desiredType)
    if type(instance) ~= 'table' then
        return false
    end
    local mt = getmetatable(instance)
    while mt ~= nil do
        if mt == desiredType then
            return true
        end
        local submt = getmetatable(mt)
        if submt == mt then
            return false
        end
        mt = submt
    end
    return false
end

-- Simple target
Target = { }
_meiqueAllTargets = {}
_meiqueCurrentDir = {'.'}
_meiqueProjectFiles = {}

function currentDir()
    if #_meiqueCurrentDir > 1 then
        return table.concat(_meiqueCurrentDir, '/')..'/'
    else
        return './'
    end
end

function sourceDir()
    return _meiqueSourceDir;
end

function buildDir()
    return _meiqueBuildDir;
end

-- Object used for disabled scopes
_meiqueNone = {}
-- Object used for enabled scopes
_meiqueNotNone = {}

_meiqueNone.__call = function(func, ...)
    return _meiqueNone
end
_meiqueNone.__index = _meiqueNone.__call
_meiqueNone.__eval = function(table)
    return false
end
_meiqueNone.__not = function(table)
    return _meiqueNotNone
end

_meiqueNotNone.__index = function(table, key)
    local typeName = type(_G[key])
    if typeName == 'function' then
        return function(a, ...) return _G[key](...) end
    elseif typeName == 'nil' then
        return nil
    else
        return function() return _G[key] end
    end
end
_meiqueNotNone.__eval = function(table)
    return true
end
_meiqueNotNone.__not = function(table)
    return _meiqueNone
end

setmetatable(_meiqueNone, _meiqueNone)
setmetatable(_meiqueNotNone, _meiqueNotNone)

-- Scopes: build types
DEBUG = _meiqueNone
RELEASE = _meiqueNone

-- Scopes: Platforms
LINUX = _meiqueNone
UNIX = _meiqueNone
WINDOWS = _meiqueNone
MACOSX = _meiqueNone

-- Scopes: Compilers
GCC = _meiqueNone
MSVC = _meiqueNone
MINGW = _meiqueNone


-- Constants
OPTIONAL = true

-- name => defaultValue
_meiqueOptionsDefaults = {}
-- name => value
_meiqueOptionsValues = {}
function option(name, description, defaultValue)
    _meiqueOptionsDefaults[name] = defaultValue
    if not _meiqueOptionsValues[name] then
        abortIf(not _meiqueOptionsDefaults[name], "Option --"..name.." need to be set in the command line, it mean:\n  "..description)
        _meiqueOptionsValues[name] = _meiqueOptionsDefaults[name]
    end
    local lowerValue = _meiqueOptionsValues[name]:lower()
    if lowerValue == "false" or lowerValue == "off" then
        return _meiqueNone
    end
    return _meiqueOptionsValues[name]
end

function addSubdirectory(dir)
    local strDir = tostring(dir)
    table.insert(_meiqueCurrentDir, dir)
    local fileName = currentDir()..'meique.lua'
    local func, error = loadfile(fileName, fileName)
    abortIf(func == nil, error)
    table.insert(_meiqueProjectFiles, fileName)
    func()
    table.remove(_meiqueCurrentDir)
end

_meiqueAllTests = {}
function addTest(testCommand, testName)
    if instanceOf(testCommand, Executable) then
        table.insert(_meiqueAllTests, {testCommand._name, testCommand:buildDir()..testCommand._name, currentDir()})
    else
        abortIf(type(testCommand) ~= "string", "Missing test command!")
        testName = testName or 'test-'..#_meiqueAllTests
        table.insert(_meiqueAllTests, {testName, testCommand, currentDir()})
    end
end

function Target:new(name)
    abortIf(name == nil and name ~= Target, 'Target name can\'t be nil')
    o = {}
    setmetatable(o, Target)
    Target.__index = Target
    if type(name) ~= 'table' then
        o._name = name
        o._files = {}
        o._deps = {}
        o._dir = currentDir()
        o._buildDir = _meiqueBuildDir..o._dir
        o._preTargetCompileHooks = {}
        o._installFiles = {}
        o._excludeFromAll = false
        abortIf(_meiqueAllTargets[tostring(name)], "You already have a target named "..name)
        _meiqueAllTargets[tostring(name)] = o
    end
    return o
end

function _meiqueRunHooks(target)
    for i, hook in pairs(target._preTargetCompileHooks) do
        hook(target)
    end
end

function Target:name()
    return self._name
end

function Target:sourceDir()
    return _meiqueSourceDir..self._dir
end

function Target:buildDir()
    return self._buildDir
end

function Target:excludeFromAll()
    self._excludeFromAll = true
end

function Target:addFile(file)
    table.insert(self._files, file)
end

function Target:addFiles(...)
    for i,file in ipairs(arg) do
        string.gsub(file, '([^%s]+)', function(f) table.insert(self._files, f) end)
    end
end

function Target:addDependency(target)
    abortIf(not instanceOf(target, Target), 'Expected a target, got something different.')
    table.insert(self._deps, target._name)
end

function Target:install(srcs, destDir, newName)
    if destDir then
        -- Custom install
        table.insert(self._installFiles, {srcs, destDir, newName} )
    else
        -- Target install
        table.insert(self._installFiles, {srcs})
    end
end

-- Custom target
CustomTarget = Target:new(Target)

function CustomTarget:new(name, func)
    o = Target:new(name)
    setmetatable(o, self)
    self.__index = self
    o._func = func
    o._type = 3
    o._outputs = {}
    return o
end

function CustomTarget:addOutput(output)
    table.insert(self._outputs, output)
end

function CustomTarget:addOutputs(...)
    for i,file in ipairs(arg) do
        string.gsub(file, '([^%s]+)', function(f) table.insert(self._outputs, f) end)
    end
end

-- Compilable target
CompilableTarget = Target:new(Target)

_meiqueGlobalPackage = {cflags = '',
                        includePaths = '',
                        libraryPaths = '',
                        linkLibraries = '',
                        linkerFlags = ''}

function CompilableTarget:new(name)
    o = Target:new(name)
    setmetatable(o, self)
    self.__index = self
    o._incDirs = {}
    o._libDirs = {}
    o._linkLibraries = {}
    -- Default package
    o._packages = { {
                    cflags = '',
                    includePaths = '',
                    libraryPaths = '',
                    linkLibraries = '',
                    linkerFlags = '',
                      } }
    o._targets = {}
    return o
end

function CompilableTarget:addIncludePath(...)
    for i,file in ipairs(arg) do
        string.gsub(file, '([^%s]+)', function(f) table.insert(self._incDirs, f) end)
    end
end

function CompilableTarget:addLibraryPath(path)
    table.insert(self._libDirs, path)
end

function CompilableTarget:addLibraryPaths(...)
    for i,file in ipairs(arg) do
        string.gsub(file, '([^%s]+)', function(f) table.insert(self._libDirs, f) end)
    end
end

function CompilableTarget:addLinkLibraries(...)
    for i,file in ipairs(arg) do
        string.gsub(file, '([^%s]+)', function(f) table.insert(self._linkLibraries, f) end)
    end
end

function addCustomFlags(self, flags)
    local pkg = nil
    if instanceOf(self, CompilableTarget) then
        pkg = self._packages[1]
    else
        pkg = _meiqueGlobalPackage
        flags = self
    end
    pkg.cflags = pkg.cflags..flags..' '
end
CompilableTarget.addCustomFlags = addCustomFlags

function CompilableTarget:use(object)
    if instanceOf(object, Library) then
        table.insert(self._targets, object._name)
        self:addDependency(object)
    elseif type(object) == 'table' then
        table.insert(self._packages, object)
    else
        error('Expected package or Library target on CompilableTarget:use(), got \''..type(object)..'\'', 0)
    end
end

-- Executable
Executable = CompilableTarget:new(Target)
function Executable:new(name)
    o = CompilableTarget:new(name)
    setmetatable(o, self)
    self.__index = self
    o._type = 1
    o._output = name
    return o
end

SHARED = 1
STATIC = 2
-- Library
Library = CompilableTarget:new(Target)
function Library:new(name, libType)
    o = CompilableTarget:new(name)
    setmetatable(o, self)
    self.__index = self
    o._libType = libType or SHARED
    o._type = 2
    if o._libType == SHARED then
        o._output = "lib"..name..".so"
    else
        o._output = "lib"..name..".a"
    end
    return o
end


-- Qt extensions

function CompilableTarget:useQtAutomoc()
    table.insert(self._preTargetCompileHooks, _meiqueAutomoc)
end

function CompilableTarget:addQtResource(...)
    table.insert(self._preTargetCompileHooks, _meiqueQtResource)
    self._qrcFiles = self._qrcFiles or {}

    for i,file in ipairs(arg) do
        string.gsub(file, '([^%s]+)', function(f) table.insert(self._qrcFiles, f) end)
    end

end
