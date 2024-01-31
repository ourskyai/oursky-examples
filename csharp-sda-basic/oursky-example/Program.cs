using OurSky.Sda.Api.Api;
using OurSky.Sda.Api.Client;
using OurSky.Sda.Api.Model;

class Program{
    static void Main(string[] args){
        var apiKey = Environment.GetEnvironmentVariable("OURSKY_API_TOKEN") ?? throw new Exception("OURSKY_API_TOKEN is not set");
        var defaultHeaders = new Dictionary<string, string> { { "Authorization", $"Bearer {apiKey}" } };

        var configuration = new Configuration
        {
            DefaultHeaders = defaultHeaders
        };

        var sdaClient = new DefaultApi(configuration);

        // fetch the iss target
        var targets = sdaClient.V1GetSatelliteTargets(noradId: "25544", orbitType: OrbitType.LEO).Targets;

        Console.WriteLine("==================== ISS Target ====================");        
        foreach (var target in targets)
        {
            Console.WriteLine(target);
        }

        var iss = targets.First();

        // fetch the upcoming potential observation windows for the iss
        var upcomingPasses = sdaClient.V1GetSatellitePotentials(
            satelliteTargetId: iss.Id,
            until: DateTime.UtcNow.AddHours(48)
        );

        Console.WriteLine("==================== Upcoming Passes ====================");
        foreach (var pass in upcomingPasses) {
            Console.WriteLine(pass);
        }

        // add a task request to observe the iss
        var orgTarget = sdaClient.V1CreateOrganizationTarget(v1CreateOrganizationTargetRequest: new V1CreateOrganizationTargetRequest(
            satelliteTargetId: iss.Id
        ));

        // fetch an observation sequence result - Note: this will only work if the task request has been completed
        // wait a day or two after submitting the task request to see if we have results for your target!
        var observationSequence = sdaClient.V1GetObservationSequenceResults(
            targetId: iss.Id
        );

        Console.WriteLine("==================== Observation Sequence ====================");
        foreach (var sequence in observationSequence) {
            Console.WriteLine(sequence);

            // fetch the node information for this OSR
            foreach (var imageSet in sequence.ImageSets) {
                var nodes = sdaClient.V1GetNodeProperties(
                    nodeId: imageSet.NodeId
                );
                Console.WriteLine(nodes);
            }
        }
    }
}
