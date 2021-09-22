//region chart variables
var map;

var height = 586
var width = 904

var dataLoc=[]
var dataLocTest = [[0,0], //where 0,0 should be
                [-78.865791,43.897], //Oshawa
                [151.209290,-33.86882], //Syndey
                [37.617298,55.755825], //Moscow
                [18.423300,-33.918861] // Cape Town
]

let startYear=1900;
let endYear=2021;

//Gets called when the page is loaded.
function init(){
  //setting up region chart and axis.
  map = d3.select('#vis').append('svg')
    .style("width",width + 'px')
    .style("height",height + 'px');


  d3.json('data/customNOANT.geo.json').then(function(countries){
    let continents = [...new Set(countries.features.map(f => f.properties.continent))];
    continents.forEach(c => {
      let cb = document.createElement("input");
      let lbl = document.createElement("label");

      cb.setAttribute("type", "checkbox");
      cb.setAttribute("id", "cb_"+c);
      cb.checked = true;

      lbl.innerText = c;
      lbl.setAttribute("for", "cb_"+c);

      document.getElementById("checkboxes").appendChild(cb);
      document.getElementById("checkboxes").appendChild(lbl);
    });

    var data = [1900, 1920, 1940, 1960, 1980, 2000, 2020];

    var sliderSimple = d3
        .sliderBottom()
        .min(d3.min(data))
        .max(d3.max(data))
        .width(300)
        .ticks(7)
        .on('onchange', val => {
          startYear = val;
        });
    var sliderSimple2 = d3
        .sliderBottom()
        .min(d3.min(data))
        .max(d3.max(data))
        .width(300)
        .ticks(7)
        .on('onchange', val => {
          endYear = val;
        });

    var gSimple = d3
        .select('div#slider-simple')
        .append('svg')
        .attr('width', 500)
        .attr('height', 100)
        .append('g')
        .attr('transform', 'translate(30,30)');
    var gSimple2 = d3
        .select('div#slider-simple2')
        .append('svg')
        .attr('width', 500)
        .attr('height', 100)
        .append('g')
        .attr('transform', 'translate(30,30)');

    gSimple.call(sliderSimple);
    gSimple2.call(sliderSimple2);

  });
}

//Called when the update button is clicked
function updateClicked(){
  map.selectAll('sites.circle').remove();
  map.selectAll('test.circle').remove();
  d3.select('#vis').selectAll("*").remove();
  map = d3.select('#vis').append('svg')
      .style("width",width + 'px')
      .style("height",height + 'px');

  d3.json('data/customNOANT.geo.json').then(function(countries){
    countries.features = countries.features.filter(c => {
      return document.getElementById("cb_"+c.properties.continent).checked
    });

    //Converts latitude and longitude to pixel positions
    projection = d3.geoMercator().fitExtent([[0,0],[width,height]],countries)
    pathGenerator = d3.geoPath().projection(projection)
    mapPath = pathGenerator(countries)

    //Converts geojson to map on svg
    map.selectAll('path')
      .data(countries.features)
      .enter()
      .append('path')
      .attr('d',pathGenerator)
      .attr('fill','none')
      .attr('stroke','#999999')
      .attr('stroke-width','1')

      //selected options for category
    var choice=[]
    var optionArea = document.getElementById('catDropdown')
    for (var i=0;i<optionArea.options.length;i++){
      if(optionArea.options[i].selected==true){
        choice.push(optionArea.options[i].value)
      }
    }

    //load in launch site data
    d3.csv("data/SummarizedYearData.csv").then(function(data){
      //create dictionary of stored csv data
      var dataDict={}
      for (var i=0;i<data.length; i++){
        var year = Number(data[i]["Year"])
        if(year>startYear&&year<endYear){
          var launchSite = data[i]["launch_site"]
          if(dataDict[launchSite]==undefined){
            var site_coords = [
              Number(data[i]["Longitude"]),
              Number(data[i]["Latitude"])
            ]
            dataDict[launchSite]={}
            dataDict[launchSite]["Military"]=0
            dataDict[launchSite]["Atmosphere"]=0
            dataDict[launchSite]["Deep Space"]=0
            dataDict[launchSite]["High Altitude"]=0
            dataDict[launchSite]["Orbital"]=0
            dataDict[launchSite]["Reentry"]=0
            dataDict[launchSite]["Suborbital"]=0
            dataDict[launchSite]["Suborbital Human Crew"]=0
            dataDict[launchSite]["Test"]=0
            dataDict[launchSite]["Coords"]=projection(site_coords)
            dataDict[launchSite]["shortName"] = data[i]["launch_site"]
            dataDict[launchSite]["fullName"] = data[i]["site_name"]
            dataDict[launchSite]["Success"]=0
            dataDict[launchSite]["Failure"]=0
            dataDict[launchSite]["Unknown"]=0
            dataDict[launchSite]["Pad Explosion"]=0
            dataDict[launchSite]["Total"]=0
          }
          var category = data[i]["Category"]
          dataDict[launchSite][category]+=Number(data[i]["Count"])
          if(choice.includes(category)){
            var results = data[i]["Result"]
            dataDict[launchSite][results]+=Number(data[i]["Count"])
            var amount = Number(data[i]["Count"])
            dataDict[launchSite]["Total"]+=amount
          }
          
        }
      }

      //create data to be used only for points where launches occured
      //given the filters
      iterableDict = d3.entries(dataDict)
      cleanedData = []
      for (var i=0;i<iterableDict.length; i++){
        if(iterableDict[i]["value"]["Total"]>0){
          cleanedData.push(iterableDict[i])
        }
      }
      //scale forthe points
      radiusScale = d3.scaleLinear()
                    .domain([0,d3.max(iterableDict,function(d){return d["value"]["Total"]})])
                    .range([1.5,20])
        
        map.selectAll('circle').remove()

        var sum = function(d){return d["value"]["Total"]}

      //generate map points
        map.selectAll('circle')
          .data(cleanedData)
          .enter()
          .append("circle")					
          .attr("cx",function(d){return d["value"]["Coords"][0];})
          .attr("cy",function(d){return d["value"]["Coords"][1];})
          .attr("r",function(d){return radiusScale(d["value"]["Total"])})
          .style("opacity",0.75)
          .style("fill","#22bb22")
          .append("title")
          .text(function(d){return toText(d)})
    })
  })
  
}

//returns string of all data to be shown in text area
function toText(inst){
  var shownText = inst["value"]["shortName"] + " - " + inst["value"]["fullName"] + 
  "\nTotal Launches: "+inst["value"]["Total"] + 
  "\nSuccess: "+inst["value"]["Success"] + 
  "\nFailure: "+inst["value"]["Failure"]+
  "\nUnknown: "+inst["value"]["Unknown"]+
  "\nPad Explosion: "+inst["value"]["Pad Explosion"]
  return shownText
}
