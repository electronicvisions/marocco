window.addEventListener("load", () => {
  let inputFile = undefined;
  
  const filereader = new FileReader();
  filereader.addEventListener("load", event => {
    visualizeGraph(event.target.result);
  });
  filereader.addEventListener("error", () => {
    console.error(`File could not be read! Code $event.target.error.code`);
  });
  
  const fileBrowser = document.querySelector("#fileBrowser");
	fileBrowser.addEventListener("change", () => {
    inputFile = fileBrowser.files[0]
	}, false);
  
  const uploadButton = document.querySelector("#upload");
  uploadButton.addEventListener("click", () => {
    if(inputFile === undefined) {
      alert("no file selected")
    } else {
      filereader.readAsText(inputFile);
    }
  })

});

// use D3 to visualize the graph
// modified example from https://github.com/ninjaconcept/d3-force-directed-graph/blob/master/example/3-user-interaction.html
const visualizeGraph = xml => {
  var [keys, nodes, links] = graphml2d3(xml);

  // manually add required strength between nodes
  for (const link of links) {
    link["strength"] = 0.7;
  }

  function getNeighbors(node) {
    return links.reduce(function (neighbors, link) {
        if (link.target.id === node.id) {
          neighbors.push(link.source.id)
        } else if (link.source.id === node.id) {
          neighbors.push(link.target.id)
        }
        return neighbors
      },
      [node.id]
    )
  }
  function isNeighborLink(node, link) {
    return link.target.id === node.id || link.source.id === node.id
  }
  function getNodeColor(node, neighbors) {
    if (Array.isArray(neighbors) && neighbors.indexOf(node.id) > -1) {
      return node.level === 1 ? 'blue' : 'green'
    }
    return node.level === 1 ? 'red' : 'gray'
  }
  function getLinkColor(node, link) {
    return isNeighborLink(node, link) ? 'green' : '#E5E5E5'
  }
  function getTextColor(node, neighbors) {
    return Array.isArray(neighbors) && neighbors.indexOf(node.id) > -1 ? 'green' : 'black'
  }
  function getText(node, selectedNode) {
    if (node.id === selectedNode.id) {
      return node.id;
    } else {
      return "";
    }
  }
  var width = window.innerWidth
  var height = window.innerHeight
  var svg = d3.select('svg')
  svg.attr('width', width).attr('height', height)
  svg.append("rect")
      .attr("fill", "none")
      .attr("pointer-events", "all")
      .attr("width", width)
      .attr("height", height)
      .call(d3.zoom()
          .scaleExtent([0.0001,8])
          .on("zoom", zoom));
  
  function zoom() {
    nodeElements.attr("transform", d3.event.transform);
    textElements.attr("transform", d3.event.transform);
    linkElements.attr("transform", d3.event.transform);
  }
  
  // simulation setup with all forces
  var linkForce = d3
    .forceLink()
    .id(function (link) { return link.id })
    .strength(function (link) { return link.strength })
  var simulation = d3
    .forceSimulation()
    .force('link', linkForce)
    .force('charge', d3.forceManyBody().strength(-120))
    .force('center', d3.forceCenter(width / 2, height / 2))
  var dragDrop = d3.drag().on('start', function (node) {
    node.fx = node.x
    node.fy = node.y
  }).on('drag', function (node) {
    simulation.alphaTarget(0.7).restart()
    node.fx = d3.event.x
    node.fy = d3.event.y
  }).on('end', function (node) {
    if (!d3.event.active) {
      simulation.alphaTarget(0)
    }
    node.fx = null
    node.fy = null
  })
  function selectNode(selectedNode) {
    var neighbors = getNeighbors(selectedNode)
    // we modify the styles to highlight selected nodes
    nodeElements.attr('fill', function (node) { return getNodeColor(node, neighbors) })
    textElements.attr('fill', function (node) { return getTextColor(node, neighbors) })
    linkElements.attr('stroke', function (link) { return getLinkColor(selectedNode, link) })
    // show the nodes ID when selected
    textElements.text(function (node) { return getText(node, selectedNode) })
  }
  var linkElements = svg.append("g")
    .attr("class", "links")
    .selectAll("line")
    .data(links)
    .enter().append("line")
      .attr("stroke-width", 1)
      .attr("stroke", "rgba(50, 50, 50, 0.2)")
  var nodeElements = svg.append("g")
    .attr("class", "nodes")
    .selectAll("circle")
    .data(nodes)
    .enter().append("circle")
      .attr("r", 10)
      .attr("fill", getNodeColor)
      .call(dragDrop)
      .on('click', selectNode)
  var textElements = svg.append("g")
    .attr("class", "texts")
    .selectAll("text")
    .data(nodes)
    .enter().append("text")
      .text("")
      .attr("font-size", 15)
      .attr("dx", 15)
      .attr("dy", 4)
  simulation.nodes(nodes).on('tick', () => {
    nodeElements
      .attr('cx', function (node) { return node.x })
      .attr('cy', function (node) { return node.y })
    textElements
      .attr('x', function (node) { return node.x })
      .attr('y', function (node) { return node.y })
    linkElements
      .attr('x1', function (link) { return link.source.x })
      .attr('y1', function (link) { return link.source.y })
      .attr('x2', function (link) { return link.target.x })
      .attr('y2', function (link) { return link.target.y })
  })
  simulation.force("link").links(links)
}

// convert graphml file-format into d3 compatible objects
const graphml2d3 = graphmlString => {
  const keys = {};
  const nodes = [];
  const edges = [];

  // parse XML to Javascript object
  // using https://github.com/nashwaan/xml-js
  const graphmlObject = JSON.parse(xml2json(graphmlString, {compact: false, spaces: 4}));
  const graphmlElement = graphmlObject.elements[0];
  let graphElement = undefined;
  for (const element of graphmlElement.elements) {
    if (element.name === "key") {
      const key = {};
      for (const i in element.attributes) {
        key[i] = element.attributes[i];
      };
      keys[key.id] = key;
    }
    if (element.name === "graph") {
      graphElement = element;
      break;
    }
  }

  const elements = graphElement.elements

  for (const element of elements) {
    if (element.name === "node") {
      const node = {};
      for (const i in element.attributes) {
        node[i] = element.attributes[i];
      }
      if (element.elements) {
        for (const infoElement of element.elements) {
          if (infoElement.name === "data") {
            node[keys[infoElement.attributes["key"]]["attr.name"]] = infoElement.elements[0].text;
          }
        }
      }
      nodes.push(node);
    }
    if (element.name === "edge") {
      const edge = {};
      for (const i in element.attributes) {
        edge[i] = element.attributes[i];
      }
      edges.push(edge);
    }
  }

  return([keys, nodes, edges]);
}