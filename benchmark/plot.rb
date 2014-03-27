require "tempfile"

def extractData(n, fileName, results)
    tool = nil
    test = nil
    File.open(fileName, "r").each do |line|
        if line =~ /(\w+): (.*)/
            tool = $1
            test = $2
            results[test] = {} unless results.key?(test)
            results[test][tool] = {} unless results[test].key?(tool)
            results[test][tool][n] = []
        elsif line =~ /real (.*)/
            results[test][tool][n].push $1.to_f;
        end
    end
end

chartTemplate = <<-eos
$('#\@CHARTID@').highcharts({
    chart: {
        type: 'spline',
    },
    title: {
        text: '@CHARTTITLE@'
    },
    subtitle: {
        text: '5 modules depending on a single module'
    },
    xAxis: {
        reversed: false,
        title: {
            enabled: true,
            text: 'Number of files per module'
        },
        labels: {
            formatter: function() {
                return this.value;
            }
        },
    },
    yAxis: {
        title: {
            text: 'seconds'
        },
    },
    legend: {
        enabled: true
    },
    tooltip: {
        headerFormat: '<b>{series.name}</b><br/>',
        pointFormat: '{point.x} file per module,<br />{point.y} seconds'
    },
    @CHATSERIES@
    });
eos

abort if ARGV.empty?

results = {}

Dir["#{ARGV[0]}out-*.txt"].each do |fileName|
    /out-(\d+)\.txt/ =~ fileName
    extractData($1.to_i, fileName, results)
end

chartData = {}
# generate plot files
results.each do |test, testData|
    key = test.gsub(".", "").gsub(" ", "_")
    template = chartTemplate.dup
    chartData[key] = template

    template.gsub!("@CHARTID@", key)
    template.gsub!("@CHARTTITLE@", test)

    series = "series: ["

    testData.each do |tool, toolData|
        series += "{\n        name: \"#{tool.gsub("_", "/")}\",\n"
        series += "        data: ["
        toolData.keys.sort.each do |n|
            values = toolData[n]
            series += "[#{n}, #{(values.inject(0.0) { |sum, el| sum + el } / values.size)}], "
        end
        series += "]\n        },"
    end
    series += "]"
    template.gsub!("@CHATSERIES@", series)
end

puts <<-eos
<html>
<script src="http://code.jquery.com/jquery-1.11.0.min.js"></script>
<script src="http://code.highcharts.com/highcharts.js"></script>
<script src="http://code.highcharts.com/modules/exporting.js"></script>
<script>
$(function () {
eos
puts chartData.values.join
puts <<-eos
});
</script>
<body>
eos
chartData.keys.each do |key|
    puts "<div id=\"#{key}\" style=\"min-width: 310px; height: 400px; margin: 0 auto\"></div>"
end
puts <<-eos
</body>
</html>
eos
