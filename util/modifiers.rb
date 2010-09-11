#!/usr/bin/env ruby

results = { }

Dir.glob("../resources/*.{js,xml}") { |f| File.open(f) { |io|
    io.readlines.each { |line|
        q = line.chomp.match(/\/\/ *@([^ ]*) +([^,]*), *([^,]*), *([^,]*)/)
        next unless q
        c = q.captures
        ((results["#{c[0]} #{c[1]}"] ||= { })[c[2].to_i] ||= []) << c[3]
    }
}}

results.sort.each { |a, b|
    puts "#{a}:"
    b.sort.each { |a, b| puts "    #{a}: #{b.join(', ')}" }
    puts
}
